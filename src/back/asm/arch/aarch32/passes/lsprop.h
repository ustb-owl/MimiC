#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LSPROP_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LSPROP_H_

#include <unordered_map>

#include "back/asm/mir/pass.h"
#include "back/asm/arch/aarch32/instdef.h"

namespace mimic::back::asmgen::aarch32 {

/*
  this pass will convert redundant load to move

  TODO: handle ldrb/strb
*/
class LoadStorePropagationPass : public PassInterface {
 public:
  LoadStorePropagationPass() {}

  void RunOn(const OprPtr &func_label, InstPtrList &insts) override {
    Reset();
    // traverse all instructions
    for (auto it = insts.begin(); it != insts.end();) {
      auto inst = static_cast<AArch32Inst *>(it->get());
      // handle by opcode
      switch (inst->opcode()) {
        case OpCode::LDR: {
          auto load = *it;
          const auto &mem_opr = load->oprs()[0].value();
          if (mem_opr->IsLabel()) {
            // try to remove redundant label load
            if (GetMemOpr(inst->dest()) == mem_opr) {
              it = insts.erase(it);
              continue;
            }
            // update label definition
            RemoveLabelDef(load->dest());
            AddLabelDef(load->dest(), mem_opr);
          }
          else {
            // try to replace with move
            auto mem = GetMemOpr(mem_opr);
            if (auto val = GetDef(mem)) {
              if (val != inst->dest()) {
                *it = std::make_shared<AArch32Inst>(OpCode::MOV,
                                                    inst->dest(), val);
              }
              else {
                it = insts.erase(it);
                continue;
              }
            }
            // update definition
            RemoveLabelDef(load->dest());
            RemoveDef(mem);
            RemoveDef(load->dest());
            RemoveUsedByDef(load->dest());
            AddDef(mem, load->dest());
          }
          break;
        }
        case OpCode::STR: {
          // update definition
          const auto &mem_opr = GetMemOpr(inst->oprs()[1].value());
          const auto &val_opr = inst->oprs()[0].value();
          RemoveDef(mem_opr);
          AddDef(mem_opr, val_opr);
          break;
        }
        case OpCode::STRB: {
          // invalidate definition
          RemoveDef(GetMemOpr(inst->oprs()[1].value()));
        }
        default: {
          if (inst->IsLabel() || inst->IsCall()) {
            Reset();
          }
          else if (inst->dest()) {
            // invalidate definition
            RemoveLabelDef(inst->dest());
            RemoveDef(inst->dest());
            RemoveUsedByDef(inst->dest());
          }
          break;
        }
      }
      ++it;
    }
  }

 private:
  using OpCode = AArch32Inst::OpCode;

  void Reset() {
    defs_.clear();
    labels_.clear();
    uses_.clear();
  }

  void RemoveLabelDef(const OprPtr &dest) {
    labels_.erase(dest);
  }

  const OprPtr &GetMemOpr(const OprPtr &mem_opr) {
    auto it = labels_.find(mem_opr);
    return it != labels_.end() ? it->second : mem_opr;
  }

  void AddLabelDef(const OprPtr &reg, const OprPtr &label) {
    assert(label->IsLabel() && reg->IsReg());
    labels_.insert({reg, label});
  }

  void RemoveDef(const OprPtr &dest) {
    auto it = defs_.find(dest);
    if (it != defs_.end()) {
      // remove from uses map
      auto [cur, end] = uses_.equal_range(it->second);
      for (; cur != end; ++cur) {
        if (cur->second == dest) {
          uses_.erase(cur);
          break;
        }
      }
      // remove from definitions
      defs_.erase(it);
    }
  }

  void RemoveUsedByDef(const OprPtr &opr) {
    auto [begin, end] = uses_.equal_range(opr);
    // remove all definitions with specific value
    for (auto it = begin; it != end; ++it) {
      defs_.erase(it->second);
    }
    // remove expired uses
    uses_.erase(begin, end);
  }

  OprPtr GetDef(const OprPtr &mem_opr) {
    auto it = defs_.find(mem_opr);
    return it != defs_.end() ? it->second : nullptr;
  }

  void AddDef(const OprPtr &dest, const OprPtr &val) {
    defs_.insert({dest, val});
    uses_.insert({val, dest});
  }

  // all value definitions
  std::unordered_map<OprPtr, OprPtr> defs_, labels_;
  // values used by definitons
  std::unordered_multimap<OprPtr, OprPtr> uses_;
};

}  // mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LSPROP_H_
