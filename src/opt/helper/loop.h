#ifndef MIMIC_OPT_PASSES_HELPER_LOOP_H_
#define MIMIC_OPT_PASSES_HELPER_LOOP_H_

#include "opt/pass.h"
#include "opt/analysis/loopinfo.h"

namespace mimic::opt {

// helper class for creating loop preheader
class PreheaderCreator : public HelperPass {
 public:
  // create preheader block for specific loop
  // if there is already one, just return it's pointer
  mid::BlockPtr CreatePreheader(const LoopInfo &loop);

  void RunOn(mid::BranchSSA &ssa) override;
  void RunOn(mid::JumpSSA &ssa) override;

 private:
  void ReroutePhi(const LoopInfo &loop);

  mid::BlockPtr preheader_;
  mid::BlockSSA *loop_entry_;
};

}  // namespace mimic::opt

#endif  // MIMIC_OPT_PASSES_HELPER_LOOP_H_
