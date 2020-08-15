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
  void InsertBefore(InstPtrList &insts, InstIt &pos, Args &&... args) {
    auto inst = std::make_shared<AArch32Inst>(std::forward<Args>(args)...);
    pos = ++insts.insert(pos, std::move(inst));
  }

  // insert signed higher 32-bit multiplication
  OprPtr InsertMulSH(InstPtrList &insts, InstIt &pos, const OprPtr &x,
                     std::int32_t imm) {
    auto y = gen_.GetVReg(), dest = gen_.GetVReg();
    InsertBefore(insts, pos, OpCode::MOV, y, gen_.GetImm(imm));
    InsertBefore(insts, pos, OpCode::SMMUL, dest, x, y);
    return dest;
  }

  // insert unsigned higher 32-bit multiplication
  OprPtr InsertMulUH(InstPtrList &insts, InstIt &pos, const OprPtr &x,
                     std::uint32_t imm) {
    auto y = gen_.GetVReg(), dest = gen_.GetVReg();
    InsertBefore(insts, pos, OpCode::MOV, y, gen_.GetImm(imm));
    InsertBefore(insts, pos, OpCode::UMULL, y, dest, x, y);
    return dest;
  }

  // insert logical right shift
  OprPtr InsertLSR(InstPtrList &insts, InstIt &pos, const OprPtr &x,
                   std::int32_t amt) {
    if (!amt) return x;
    const auto &y = gen_.GetImm(amt);
    auto dest = gen_.GetVReg();
    InsertBefore(insts, pos, OpCode::LSR, dest, x, y);
    return dest;
  }

  // insert arithmetic right shift
  OprPtr InsertASR(InstPtrList &insts, InstIt &pos, const OprPtr &x,
                   std::int32_t amt) {
    if (!amt) return x;
    const auto &y = gen_.GetImm(amt);
    auto dest = gen_.GetVReg();
    InsertBefore(insts, pos, OpCode::ASR, dest, x, y);
    return dest;
  }

  // insert XSIGN
  OprPtr InsertXSign(InstPtrList &insts, InstIt &pos, const OprPtr &x) {
    return InsertASR(insts, pos, x, N - 1);
  }

  // insert add
  OprPtr InsertAdd(InstPtrList &insts, InstIt pos, const OprPtr &x,
                   const OprPtr &y) {
    auto dest = gen_.GetVReg();
    InsertBefore(insts, pos, OpCode::ADD, dest, x, y);
    return dest;
  }

  // insert sub
  OprPtr InsertSub(InstPtrList &insts, InstIt pos, const OprPtr &x,
                   const OprPtr &y) {
    auto dest = gen_.GetVReg();
    InsertBefore(insts, pos, OpCode::SUB, dest, x, y);
    return dest;
  }

  // insert negate
  OprPtr InsertNeg(InstPtrList &insts, InstIt pos, const OprPtr &x) {
    auto dest = gen_.GetVReg();
    InsertBefore(insts, pos, OpCode::RSB, dest, x, gen_.GetImm(0));
    return dest;
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
    OprPtr ans;
    if (d == (1ull << l)) {
      ans = InsertLSR(insts, pos, n, l);
    }
    else if (m >= (1ull << N)) {
      assert(sh_pre == 0);
      auto t1 = InsertMulUH(insts, pos, n, m - (1ull << N));
      ans = InsertSub(insts, pos, n, t1);
      ans = InsertLSR(insts, pos, ans, 1);
      ans = InsertAdd(insts, pos, t1, ans);
      ans = InsertLSR(insts, pos, ans, sh_post - 1);
    }
    else {
      ans = InsertLSR(insts, pos, n, sh_pre);
      ans = InsertMulUH(insts, pos, ans, m);
      ans = InsertLSR(insts, pos, ans, sh_post);
    }
    // insert move
    InsertBefore(insts, pos, OpCode::MOV, dest, ans);
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
    OprPtr ans;
    if (abs_d == 1) {
      ans = gen_.GetVReg();
      InsertBefore(insts, pos, OpCode::MOV, ans, n);
    }
    else if (abs_d == (1 << m.l)) {
      ans = InsertASR(insts, pos, n, m.l - 1);
      ans = InsertLSR(insts, pos, ans, N - m.l);
      ans = InsertAdd(insts, pos, n, ans);
      ans = InsertASR(insts, pos, ans, m.l);
    }
    else if (m.m_high < (1ull << (N - 1))) {
      ans = InsertMulSH(insts, pos, n, m.m_high);
      ans = InsertASR(insts, pos, ans, m.sh_post);
      auto t = InsertXSign(insts, pos, n);
      ans = InsertSub(insts, pos, ans, t);
    }
    else {
      ans = InsertMulSH(insts, pos, n, m.m_high - (1ull << N));
      ans = InsertAdd(insts, pos, ans, n);
      ans = InsertASR(insts, pos, ans, m.sh_post);
      auto t = InsertXSign(insts, pos, n);
      ans = InsertSub(insts, pos, ans, t);
    }
    // generate move or negate
    if (d < 0) {
      InsertBefore(insts, pos, OpCode::RSB, dest, ans, gen_.GetImm(0));
    }
    else {
      InsertBefore(insts, pos, OpCode::MOV, dest, ans);
    }
    // remove current instruction
    return insts.erase(pos);
  }

  AArch32InstGen &gen_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_DIVOPT_H_
