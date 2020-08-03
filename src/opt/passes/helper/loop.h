#ifndef MIMIC_OPT_PASSES_HELPER_LOOP_H_
#define MIMIC_OPT_PASSES_HELPER_LOOP_H_

#include <unordered_set>
#include <list>

#include "mid/ssa.h"

namespace mimic::opt {

class LoopDetector {
 public:
  // information of loop
  struct LoopInfo {
    // entry block (head of back edge)
    mid::BlockSSA *entry;
    // tail block (tail of back edge)
    mid::BlockSSA *tail;
    // all blocks of loop (containing entry and tail)
    std::unordered_set<mid::BlockSSA *> body;
  };

  using LoopInfoList = std::list<LoopInfo>;

  LoopDetector(mid::FunctionSSA *func) { ScanOn(func); }

  // getters
  const LoopInfoList &loops() const { return loops_; }

 private:
  void ScanOn(mid::FunctionSSA *func);
  void ScanNaturalLoop(mid::BlockSSA *be_tail, mid::BlockSSA *be_head);

  // all detected loops
  LoopInfoList loops_;
};

}  // namespace mimic::opt

#endif  // MIMIC_OPT_PASSES_HELPER_LOOP_H_
