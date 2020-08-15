#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_DIVOPT_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_DIVOPT_H_

#include <cstdint>
#include <cmath>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"
#include "back/asm/arch/aarch32/instgen.h"

namespace mimic::back::asmgen::aarch32 {

/*
  this pass will try to optimize divisions of variable by constant
  reference: Division by Invariant Integers using Multiplication
*/
class DivisionOptimizationPass : public PassInterface {
 public:
  DivisionOptimizationPass(AArch32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    for (auto it = insts.begin(); it != insts.end();) {
      auto inst = static_cast<AArch32Inst *>(it->get());
      if (inst->oprs().size() == 2 && inst->oprs()[1].value()->IsImm()) {
        const auto &dest = inst->dest(), &n = inst->oprs()[0].value();
        auto dp = static_cast<AArch32Imm *>(inst->oprs()[1].value().get());
        auto d = dp->val();
        // handle by opcode
        if (inst->opcode() == OpCode::SDIV) {
          it = GenerateSignedDiv(insts, it, dest, n, d);
          continue;
        }
        else if (inst->opcode() == OpCode::UDIV) {
          it = GenerateUnsignedDiv(insts, it, dest, n, d);
          continue;
        }
      }
      ++it;
    }
  }

 private:
  using OpCode = AArch32Inst::OpCode;
  using InstIt = InstPtrList::iterator;

  // bit length of machine word
  const std::size_t N = 32;

  struct Multiplier {
    std::uint64_t m_high;
    int sh_post;
    int l;
  };

  // insert instruction before the specific position
  template <typename... Args>
  InstIt InsertBefore(InstPtrList &insts, InstIt pos, Args &&... args) {
    auto inst = std::make_shared<AArch32Inst>(std::forward<Args>(args)...);
    return ++insts.insert(pos, std::move(inst));
  }

  // insert signed higher 32-bit multiplication
  InstIt InsertMulSH(InstPtrList &insts, InstIt pos, const OprPtr &dest,
                     const OprPtr &x, std::int32_t imm) {
    auto y = gen_.GetVReg();
    pos = InsertBefore(insts, pos, OpCode::MOV, y, gen_.GetImm(imm));
    return InsertBefore(insts, pos, OpCode::SMMUL, dest, x, y);
  }

  // insert unsigned higher 32-bit multiplication
  InstIt InsertMulUH(InstPtrList &insts, InstIt pos, const OprPtr &dest,
                     const OprPtr &x, std::uint32_t imm) {
    auto y = gen_.GetVReg();
    pos = InsertBefore(insts, pos, OpCode::MOV, y, gen_.GetImm(imm));
    return InsertBefore(insts, pos, OpCode::UMULL, y, dest, x, y);
  }

  // insert logical right shift
  InstIt InsertLSR(InstPtrList &insts, InstIt pos, const OprPtr &dest,
                   const OprPtr &x, std::int32_t amt) {
    if (!amt) return pos;
    const auto &y = gen_.GetImm(amt);
    return InsertBefore(insts, pos, OpCode::LSR, dest, x, y);
  }

  // insert arithmetic right shift
  InstIt InsertASR(InstPtrList &insts, InstIt pos, const OprPtr &dest,
                   const OprPtr &x, std::int32_t amt) {
    if (!amt) return pos;
    const auto &y = gen_.GetImm(amt);
    return InsertBefore(insts, pos, OpCode::ASR, dest, x, y);
  }

  // insert XSIGN
  InstIt InsertXSign(InstPtrList &insts, InstIt pos, const OprPtr &dest,
                     const OprPtr &x) {
    return InsertASR(insts, pos, dest, x, N - 1);
  }

  // insert negate
  InstIt InsertNeg(InstPtrList &insts, InstIt pos, const OprPtr &dest,
                   const OprPtr &x) {
    return InsertBefore(insts, pos, OpCode::RSB, dest, x, gen_.GetImm(0));
  }

  Multiplier ChooseMultiplier(std::uint32_t d, int prec) {
    int l = ceil(log2(d)), sh_post = l;
    std::uint64_t m_low = (1ull << (N + l)) / d, m_high;
    m_high = ((1ull << (N + l)) + (1ull << (N + l - prec))) / d;
    while (m_low / 2 < m_high / 2 && sh_post > 0) {
      m_low /= 2;
      m_high /= 2;
      sh_post -= 1;
    }
    return {m_high, sh_post, l};
  }

  InstIt GenerateUnsignedDiv(InstPtrList &insts, InstIt pos,
                             const OprPtr &dest, const OprPtr &n,
                             std::uint32_t d) {
    if (!d) return ++pos;
    Multiplier mp = ChooseMultiplier(d, N);
    auto m = mp.m_high;
    int sh_post = mp.sh_post, l = mp.l, sh_pre;
    if (m >= (1ull << N) && d % 2 == 0) {
      int e = log2(d & ((1ull << N) - d));
      auto d_odd = d >> e;
      sh_pre = e;
      mp = ChooseMultiplier(d_odd, N - e);
      m = mp.m_high;
      sh_post = mp.sh_post;
    }
    else {
      sh_pre = 0;
    }
    // generate code
    if (d == (1ull << l)) {
      pos = InsertLSR(insts, pos, dest, n, l);
    }
    else if (m >= (1ull << N)) {
      assert(sh_pre == 0);
      auto t1 = gen_.GetVReg(), t2 = gen_.GetVReg();
      pos = InsertMulUH(insts, pos, t1, n, m - (1ull << N));
      pos = InsertBefore(insts, pos, OpCode::SUB, dest, n, t1);
      pos = InsertLSR(insts, pos, t2, dest, 1);
      pos = InsertBefore(insts, pos, OpCode::ADD, dest, t1, t2);
      pos = InsertLSR(insts, pos, dest, dest, sh_post - 1);
    }
    else {
      pos = InsertLSR(insts, pos, dest, n, sh_pre);
      pos = InsertMulUH(insts, pos, dest, dest, m);
      pos = InsertLSR(insts, pos, dest, dest, sh_post);
    }
    // remove current instruction
    return insts.erase(pos);
  }

  InstIt GenerateSignedDiv(InstPtrList &insts, InstIt pos,
                           const OprPtr &dest, const OprPtr &n,
                           std::int32_t d) {
    if (!d) return ++pos;
    auto abs_d = d > 0 ? d : -d;
    Multiplier m = ChooseMultiplier(abs_d, N - 1);
    // generate code
    if (abs_d == 1) {
      pos = InsertBefore(insts, pos, OpCode::MOV, dest, n);
    }
    else if (abs_d == (1 << m.l)) {
      pos = InsertASR(insts, pos, dest, n, m.l - 1);
      auto t = gen_.GetVReg();
      pos = InsertLSR(insts, pos, t, dest, N - m.l);
      pos = InsertBefore(insts, pos, OpCode::ADD, dest, n, t);
      pos = InsertASR(insts, pos, dest, dest, m.l);
    }
    else if (m.m_high < (1ull << (N - 1))) {
      pos = InsertMulSH(insts, pos, dest, n, m.m_high);
      pos = InsertASR(insts, pos, dest, dest, m.sh_post);
      auto t = gen_.GetVReg();
      pos = InsertXSign(insts, pos, t, n);
      pos = InsertBefore(insts, pos, OpCode::SUB, dest, dest, t);
    }
    else {
      pos = InsertMulSH(insts, pos, dest, n, m.m_high - (1ull << N));
      pos = InsertBefore(insts, pos, OpCode::ADD, dest, dest, n);
      pos = InsertASR(insts, pos, dest, dest, m.sh_post);
      auto t = gen_.GetVReg();
      pos = InsertXSign(insts, pos, t, n);
      pos = InsertBefore(insts, pos, OpCode::SUB, dest, dest, t);
    }
    if (d < 0) pos = InsertNeg(insts, pos, dest, dest);
    // remove current instruction
    return insts.erase(pos);
  }

  AArch32InstGen &gen_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_DIVOPT_H_
