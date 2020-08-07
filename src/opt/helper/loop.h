#ifndef MIMIC_OPT_PASSES_HELPER_LOOP_H_
#define MIMIC_OPT_PASSES_HELPER_LOOP_H_

#include <unordered_set>
#include <list>

#include "opt/pass.h"
#include "opt/helper/dom.h"

namespace mimic::opt {

// information of loop
struct LoopInfo {
  // entry block (head of back edge)
  mid::BlockSSA *entry;
  // tail block (tail of back edge)
  mid::BlockSSA *tail;
  // all blocks of loop (containing entry and tail)
  std::unordered_set<mid::BlockSSA *> body;
  // exit blocks
  std::unordered_set<mid::BlockSSA *> exit;
};

// list of loop information
using LoopInfoList = std::list<LoopInfo>;


// detecte all loops in function
class LoopDetector {
 public:
  LoopDetector(mid::FunctionSSA *func) {
    DominanceChecker dom(func);
    ScanOn(dom, func);
  }
  LoopDetector(DominanceChecker &dom, mid::FunctionSSA *func) {
    ScanOn(dom, func);
  }

  // getters
  const LoopInfoList &loops() const { return loops_; }

 private:
  void ScanOn(DominanceChecker &dom, mid::FunctionSSA *func);
  void ScanNaturalLoop(mid::BlockSSA *be_tail, mid::BlockSSA *be_head);

  // all detected loops
  LoopInfoList loops_;
};


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
