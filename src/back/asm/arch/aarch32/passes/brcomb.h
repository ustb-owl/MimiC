#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_BRCOMB_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_BRCOMB_H_

#include <unordered_map>
#include <cassert>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"
#include "back/asm/arch/aarch32/instgen.h"

namespace mimic::back::asmgen::aarch32 {

/*
  this pass will:
  1.  combine SETC and BR
  2.  convert all SETCs and BRs to real instructions
*/
class BranchCombiningPass : public PassInterface {
 public:
  BranchCombiningPass(AArch32InstGen &gen) : gen_(gen) {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    // handle BRs
    ResetDefs();
    for (auto it = insts.begin(); it != insts.end();) {
      auto inst = static_cast<AArch32Inst *>(it->get());
      switch (inst->opcode()) {
        case OpCode::SETEQ: case OpCode::SETNE: case OpCode::SETULT:
        case OpCode::SETSLT: case OpCode::SETULE: case OpCode::SETSLE:
        case OpCode::SETUGT: case OpCode::SETSGT: case OpCode::SETUGE:
        case OpCode::SETSGE: {
          // add SETC instructions to map
          assert(inst->dest()->IsVirtual() && !setcs_.count(inst->dest()));
          AddDef(inst);
          break;
        }
        case OpCode::BR: {
          // handle branchs
          it = HandleBranch(insts, it, inst);
          continue;
        }
        default: {
          if (inst->IsLabel() || inst->IsCall()) {
            ResetDefs();
          }
          else if (inst->dest()) {
            RemoveDef(inst->dest());
            RemoveUsedByDef(inst->dest());
          }
          break;
        }
      }
      ++it;
    }
    // handle remaining SETCs
    for (auto it = insts.begin(); it != insts.end();) {
      auto inst = static_cast<AArch32Inst *>(it->get());
      switch (inst->opcode()) {
        case OpCode::SETEQ: case OpCode::SETNE: case OpCode::SETULT:
        case OpCode::SETSLT: case OpCode::SETULE: case OpCode::SETSLE:
        case OpCode::SETUGT: case OpCode::SETSGT: case OpCode::SETUGE:
        case OpCode::SETSGE: {
          it = HandleSetCond(insts, it, inst);
          continue;
        }
        default:;
      }
      ++it;
    }
  }

 private:
  using OpCode = AArch32Inst::OpCode;
  using InstIt = InstPtrList::iterator;

  void ResetDefs() {
    setcs_.clear();
    uses_.clear();
  }

  void RemoveDef(const OprPtr &dest) {
    auto it = setcs_.find(dest);
    if (it != setcs_.end()) {
      // remove from uses map
      for (const auto &opr : it->second->oprs()) {
        auto [cur, end] = uses_.equal_range(opr.value());
        for (; cur != end; ++cur) {
          if (cur->second == dest) {
            uses_.erase(cur);
            break;
          }
        }
      }
      // remove from definitions
      setcs_.erase(it);
    }
  }

  void RemoveUsedByDef(const OprPtr &opr) {
    auto [begin, end] = uses_.equal_range(opr);
    // remove all definitions with specific value
    for (auto it = begin; it != end; ++it) {
      setcs_.erase(it->second);
    }
    // remove expired uses
    uses_.erase(begin, end);
  }

  void AddDef(AArch32Inst *setc) {
    setcs_.insert({setc->dest(), setc});
    uses_.insert({setc->oprs()[0].value(), setc->dest()});
    uses_.insert({setc->oprs()[1].value(), setc->dest()});
  }

  template <typename... Args>
  InstIt InsertInst(InstPtrList &insts, InstIt pos, Args &&... args) {
    auto inst = std::make_shared<AArch32Inst>(std::forward<Args>(args)...);
    return ++insts.insert(pos, std::move(inst));
  }

  InstIt GenerateCondBranch(InstPtrList &insts, InstIt pos,
                            AArch32Inst *cond, AArch32Inst *br) {
    const auto &tl = br->oprs()[1].value(), &fl = br->oprs()[2].value();
    // generate 'CMP lhs, rhs'
    const auto &lhs = cond->oprs()[0].value();
    const auto &rhs = cond->oprs()[1].value();
    pos = InsertInst(insts, pos, OpCode::CMP, lhs, rhs);
    // generate 'BXX fl'
    OpCode bxx;
    switch (cond->opcode()) {
      case OpCode::SETEQ: bxx = OpCode::BNE; break;
      case OpCode::SETNE: bxx = OpCode::BEQ; break;
      case OpCode::SETULT: bxx = OpCode::BHS; break;
      case OpCode::SETSLT: bxx = OpCode::BGE; break;
      case OpCode::SETULE: bxx = OpCode::BHI; break;
      case OpCode::SETSLE: bxx = OpCode::BGT; break;
      case OpCode::SETUGT: bxx = OpCode::BLS; break;
      case OpCode::SETSGT: bxx = OpCode::BLE; break;
      case OpCode::SETUGE: bxx = OpCode::BLO; break;
      case OpCode::SETSGE: bxx = OpCode::BLT; break;
      default: assert(false);
    }
    pos = InsertInst(insts, pos, bxx, fl);
    // generate 'B tl'
    pos = InsertInst(insts, pos, OpCode::B, tl);
    return pos;
  }

  InstIt GenerateBranch(InstPtrList &insts, InstIt pos, AArch32Inst *br) {
    const auto &cond = br->oprs()[0].value();
    const auto &tl = br->oprs()[1].value(), &fl = br->oprs()[2].value();
    // generate 'CMP cond, 0'
    pos = InsertInst(insts, pos, OpCode::CMP, cond, gen_.GetImm(0));
    // generate branchs
    pos = InsertInst(insts, pos, OpCode::BEQ, fl);
    pos = InsertInst(insts, pos, OpCode::B, tl);
    return pos;
  }

  InstIt HandleBranch(InstPtrList &insts, InstIt pos, AArch32Inst *br) {
    // try to generate conditional branch
    const auto &cond = br->oprs()[0].value();
    auto it = setcs_.find(cond);
    if (it != setcs_.end()) {
      pos = GenerateCondBranch(insts, pos, it->second, br);
    }
    else {
      pos = GenerateBranch(insts, pos, br);
    }
    // remove current instruction
    return insts.erase(pos);
  }

  InstIt HandleSetCond(InstPtrList &insts, InstIt pos, AArch32Inst *setc) {
    if (setc->dest()->use_count()) {
      const auto &lhs = setc->oprs()[0].value();
      const auto &rhs = setc->oprs()[1].value();
      const auto &dest = setc->dest();
      auto temp = gen_.GetReg(AArch32Reg::RegName::R0);
      switch (setc->opcode()) {
        case OpCode::SETEQ: {
          pos = InsertInst(insts, pos, OpCode::SUB, temp, lhs, rhs);
          pos = InsertInst(insts, pos, OpCode::CLZ, temp, temp);
          pos = InsertInst(insts, pos, OpCode::LSR, dest, temp,
                           gen_.GetImm(5));
          break;
        }
        case OpCode::SETNE: {
          pos = InsertInst(insts, pos, OpCode::SUBS, temp, lhs, rhs);
          pos = InsertInst(insts, pos, OpCode::MOVWNE, temp,
                           gen_.GetImm(1));
          pos = InsertInst(insts, pos, OpCode::MOV, dest, temp);
          break;
        }
        default: {
          pos = InsertInst(insts, pos, OpCode::MOV, temp, gen_.GetImm(0));
          pos = InsertInst(insts, pos, OpCode::CMP, lhs, rhs);
          OpCode opcode;
          switch (setc->opcode()) {
            case OpCode::SETULT: opcode = OpCode::MOVWLO; break;
            case OpCode::SETSLT: opcode = OpCode::MOVWLT; break;
            case OpCode::SETULE: opcode = OpCode::MOVWLS; break;
            case OpCode::SETSLE: opcode = OpCode::MOVWLE; break;
            case OpCode::SETUGT: opcode = OpCode::MOVWHI; break;
            case OpCode::SETSGT: opcode = OpCode::MOVWGT; break;
            case OpCode::SETUGE: opcode = OpCode::MOVWHS; break;
            case OpCode::SETSGE: opcode = OpCode::MOVWGE; break;
            default: assert(false);
          }
          pos = InsertInst(insts, pos, opcode, temp, gen_.GetImm(1));
          pos = InsertInst(insts, pos, OpCode::MOV, dest, temp);
          break;
        }
      }
    }
    // remove current instruction
    return insts.erase(pos);
  }

  AArch32InstGen &gen_;
  std::unordered_map<OprPtr, AArch32Inst *> setcs_;
  std::unordered_multimap<OprPtr, OprPtr> uses_;
};

}  // namespace mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_BRCOMB_H_
