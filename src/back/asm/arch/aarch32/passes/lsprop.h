#ifndef MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LSELIM_H_
#define MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LSELIM_H_

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
    // traverse all instructions
    for (auto &&i : insts) {
      if (i->IsLabel() || i->IsCall()) Reset();
      auto inst = static_cast<AArch32Inst *>(i.get());
      switch (inst->opcode()) {
        case OpCode::LDR: {
          auto load = i;
          const auto &mem_opr = load->oprs()[0].value();
          // try to replace with move
          auto it = defs_.find(mem_opr);
          if (it != defs_.end()) {
            i = std::make_shared<AArch32Inst>(OpCode::MOV, inst->dest(),
                                              it->second);
          }
          // update definition
          RemoveDef(mem_opr);
          RemoveUsedByDef(load->dest());
          AddDef(mem_opr, load->dest());
          break;
        }
        case OpCode::STR: {
          // update definition
          const auto &mem_opr = inst->oprs()[1].value();
          const auto &val_opr = inst->oprs()[0].value();
          RemoveDef(mem_opr);
          AddDef(mem_opr, val_opr);
          break;
        }
        case OpCode::STRB: {
          // invalidate definition
          RemoveDef(inst->oprs()[1].value());
        }
        default: {
          // invalidate definition
          if (inst->dest()) RemoveUsedByDef(inst->dest());
          break;
        }
      }
    }
  }

 private:
  using OpCode = AArch32Inst::OpCode;

  void Reset() {
    defs_.clear();
    uses_.clear();
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

  void AddDef(const OprPtr &dest, const OprPtr &val) {
    defs_.insert({dest, val});
    uses_.insert({val, dest});
  }

  // all value definitions
  std::unordered_map<OprPtr, OprPtr> defs_;
  // values used by definitons
  std::unordered_multimap<OprPtr, OprPtr> uses_;
};

}  // mimic::back::asmgen::aarch32

#endif  // MIMIC_BACK_ASM_ARCH_AARCH32_PASSES_LSELIM_H_
