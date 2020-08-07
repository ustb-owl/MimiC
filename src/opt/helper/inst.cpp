#include "opt/helper/inst.h"

#include <cassert>

#include "opt/helper/cast.h"

using namespace mimic::mid;
using namespace mimic::opt;

void ParentScanner::RunOn(FunctionSSA &ssa) {
  // traverse all blocks to get parent relationship
  for (const auto &use : ssa) {
    auto block = SSACast<BlockSSA>(use.value().get());
    for (const auto &i : block->insts()) {
      parent_map_.insert({i.get(), block});
    }
  }
}

// get parent block of specific value
BlockSSA *ParentScanner::GetParent(Value *val) {
  auto it = parent_map_.find(val);
  if (it == parent_map_.end()) {
    // value is a sub-instruction, it's used by other instruction
    // but not directly inserted to any block
    assert(val->uses().size() == 1);
    auto parent = GetParent(val->uses().front()->user());
    parent_map_.insert({val, parent});
    return parent;
  }
  else {
    return it->second;
  }
}
