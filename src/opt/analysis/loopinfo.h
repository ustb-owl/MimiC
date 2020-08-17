#ifndef MIMIC_OPT_ANALYSIS_LOOPINFO_H_
#define MIMIC_OPT_ANALYSIS_LOOPINFO_H_

#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "opt/pass.h"

namespace mimic::opt {

// information of loop
struct LoopInfo {
  // entry block (head of back edge)
  mid::BlockSSA *entry;
  // tail block (tail of back edge)
  mid::BlockSSA *tail;
  // preheader block
  mid::BlockSSA *preheader;
  // all blocks of loop (containing entry and tail)
  std::unordered_set<mid::BlockSSA *> body;
  // exit blocks
  std::unordered_set<mid::BlockSSA *> exit;
  // induction variable
  mid::PhiSSA *ind_var;
  // initial value of induction variable
  mid::Value *init_val;
  // instruction to modify the induction variable in each loop
  mid::BinarySSA *modifier;
  // the end condition of loop
  mid::BinarySSA *end_cond;
  // block to jump when leaving loop entry
  mid::BlockSSA *first_block;
  // block to jump to when the loop exits
  mid::BlockSSA *exit_block;
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
  void ScanNaturalLoop(LoopInfoList &loops, mid::BlockSSA *be_tail,
                       mid::BlockSSA *be_head);
  void ScanPreheader(LoopInfo &info);
  void ScanExitBlocks(LoopInfo &info);
  void ScanIndVarInfo(LoopInfo &info);

  // all detected loops
  std::unordered_map<mid::FunctionSSA *, LoopInfoList> loops_;
};

}  // namespace mimic::opt

#endif  // MIMIC_OPT_ANALYSIS_LOOPINFO_H_
