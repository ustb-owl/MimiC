#ifndef MIMIC_OPT_PASSES_HELPER_DOM_H_
#define MIMIC_OPT_PASSES_HELPER_DOM_H_

#include <unordered_map>
#include <cstddef>
#include <cassert>

#include "utils/bitvec.h"

namespace mimic::opt {

class DominanceChecker {
 public:
  DominanceChecker(mid::FunctionSSA *func) {
    assert(!func->is_decl());
    SolveDominance(func);
  }

  // check if block is dead
  bool IsDeadBlock(mid::BlockSSA *block) const {
    return block_id_.count(block);
  }

  // check if block 'b1' dominates block 'b2'
  bool IsDominate(mid::BlockSSA *b1, mid::BlockSSA *b2) const;

 private:
  void SolveDominance(mid::FunctionSSA *func);

  std::unordered_map<mid::BlockSSA *, std::size_t> block_id_;
  std::unordered_map<mid::BlockSSA *, utils::BitVec> dom_;
};

}  // namespace mimic::opt

#endif  // MIMIC_OPT_PASSES_HELPER_DOM_H_
