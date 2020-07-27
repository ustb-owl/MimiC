#ifndef MIMIC_BACK_ASM_MIR_PASSES_MOVPROP_H_
#define MIMIC_BACK_ASM_MIR_PASSES_MOVPROP_H_

#include <unordered_map>

#include "back/asm/mir/pass.h"

namespace mimic::back::asmgen {

class MovePropagationPass : public PassInterface {
 public:
  MovePropagationPass() {}

  void RunOn(InstPtrList &insts) override {
    Reset();
    // traverse all instructions
    for (const auto &i : insts) {
      if (i->IsLabel() || i->IsCall()) Reset();
      // replace operands
      for (auto &&opr : i->oprs()) {
        auto it = defs_.find(opr.value());
        if (it != defs_.end()) opr.set_value(it->second);
      }
      // update definition map
      if (i->IsMove()) {
        AddDef(i->dest(), i->oprs()[0].value());
      }
      else if (i->dest()) {
        RemoveDef(i->dest());
        RemoveUsedByDef(i->dest());
      }
    }
  }

 private:
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
    RemoveDef(dest);
    defs_.insert({dest, val});
    uses_.insert({val, dest});
  }

  // all value definitions
  std::unordered_map<OprPtr, OprPtr> defs_;
  // values used by definitons
  std::unordered_multimap<OprPtr, OprPtr> uses_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASSES_MOVPROP_H_
