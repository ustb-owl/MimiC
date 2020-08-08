#ifndef MIMIC_OPT_ANALYSIS_DOMINANCE_H_
#define MIMIC_OPT_ANALYSIS_DOMINANCE_H_

#include <unordered_map>
#include <cstddef>
#include <cassert>

#include "opt/pass.h"
#include "utils/bitvec.h"

namespace mimic::opt {

/*
  this pass will analysis dominance information
*/
class DominanceInfoPass : public FunctionPass {
 public:
  DominanceInfoPass() {}

  bool RunOnFunction(const mid::FuncPtr &func) override;
  void Initialize() override;

  // check if block is dead
  bool IsDeadBlock(mid::BlockSSA *block) const;

  // check if block 'b1' dominates block 'b2'
  bool IsDominate(mid::BlockSSA *b1, mid::BlockSSA *b2) const;

 private:
  // dominance information (per function)
  struct DominanceInfo {
    // id of all basic blocks
    std::unordered_map<mid::BlockSSA *, std::size_t> block_id;
    // dominance set of basic blocks
    std::unordered_map<mid::BlockSSA *, utils::BitVec> dom;
  };

  const DominanceInfo &GetDominanceInfo(mid::BlockSSA *block) const;
  void SolveDominance(const mid::FuncPtr &func);

  std::unordered_map<mid::User *, DominanceInfo> dom_info_;
};

}  // namespace mimic::opt

#endif  // MIMIC_OPT_ANALYSIS_DOMINANCE_H_
