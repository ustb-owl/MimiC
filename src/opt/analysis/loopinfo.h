#ifndef MIMIC_OPT_PASSES_ANALYSIS_LOOPINFO_H_
#define MIMIC_OPT_PASSES_ANALYSIS_LOOPINFO_H_

#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "opt/pass.h"
#include "opt/analysis/dominance.h"

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
using LoopInfoList = std::vector<LoopInfo>;


/*
  this pass will detecte all loops in function
*/
class LoopInfoPass : public FunctionPass {
 public:
  LoopInfoPass() {}

  bool RunOnFunction(const mid::FuncPtr &func) override;
  void Initialize() override;

  // get information of all loops in a specific function
  const LoopInfoList &GetLoopInfo(mid::FunctionSSA *func) const {
    auto it = loops_.find(func);
    assert(it != loops_.end());
    return it->second;
  }

 private:
  void ScanOn(const mid::FuncPtr &func);
  void ScanNaturalLoop(const mid::FuncPtr &func, mid::BlockSSA *be_tail,
                       mid::BlockSSA *be_head);

  // dominance info
  const DominanceInfoPass *dom_;
  // all detected loops
  std::unordered_map<mid::FunctionSSA *, LoopInfoList> loops_;
};

}  // namespace mimic::opt

#endif  // MIMIC_OPT_PASSES_ANALYSIS_LOOPINFO_H_
