#ifndef MIMIC_OPT_PASSES_HELPER_INST_H_
#define MIMIC_OPT_PASSES_HELPER_INST_H_

#include <unordered_map>

#include "opt/pass.h"

namespace mimic::opt {

// helper pass for getting parent block of instructions in function
class ParentScanner : public HelperPass {
 public:
  ParentScanner(const mid::UserPtr &func) { func->RunPass(*this); }
  ParentScanner(mid::FunctionSSA *func) { func->RunPass(*this); }

  void RunOn(mid::FunctionSSA &ssa) override;

  // get parent block of specific value
  mid::BlockSSA *GetParent(mid::Value *val);
  // update parent of value
  void UpdateParent(mid::Value *val, mid::BlockSSA *parent) {
    parent_map_[val] = parent;
  }

 private:
  std::unordered_map<mid::Value *, mid::BlockSSA *> parent_map_;
};

}  // namespace mimic::opt

#endif  // MIMIC_OPT_PASSES_HELPER_INST_H_
