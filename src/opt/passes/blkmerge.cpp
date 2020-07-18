#include "opt/pass.h"
#include "opt/passman.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  merge blocks that connected with jump instructions

  e.g.
    %0:
      ...                         %0:
      jump %1                       ...
    %1: ; preds: %0                 ...
      ...                 ==>>      jump %2
      jump %2                     %2: ; preds: %0, %3
    %2: ; preds: %1, %3             ...
      ...                           jump %4
      jump %4

*/
class BlockMergePass : public FunctionPass {
 public:
  BlockMergePass() {}

  bool RunOnFunction(const UserPtr &func) override {
    SSAPtr entry;
    changed_ = false;
    // traverse all basic blocks
    for (auto &&i : *func) {
      // record entry block
      if (!entry) entry = i.value();
      // run on current block
      i.value()->RunPass(*this);
      if (merge_flag_) {
        // update 'changed' flag
        if (!changed_) changed_ = true;
        // replace target block by current block
        target_->ReplaceBy(i.value());
        // set current value to nullptr
        i.set_value(nullptr);
      }
    }
    // rearrange blocks
    if (changed_) {
      func->RemoveValue(nullptr);
      for (std::size_t i = 0; i < func->size(); ++i) {
        if ((*func)[i].value() == entry) {
          // place entry block at the beginning of use list
          if (i) {
            (*func)[i].set_value((*func)[0].value());
            (*func)[0].set_value(entry);
          }
          break;
        }
      }
    }
    return changed_;
  }

  void RunOn(BlockSSA &ssa) override {
    // check the last instruction
    merge_flag_ = false;
    ssa.insts().back()->RunPass(*this);
    // check if need to merge
    if (merge_flag_) {
      // remove the last jump
      ssa.insts().pop_back();
      // insert instructions from target block
      ssa.insts().insert(ssa.insts().end(), target_->insts().begin(),
                         target_->insts().end());
    }
  }

  void RunOn(JumpSSA &ssa) override {
    // get target
    target_ = static_cast<BlockSSA *>(ssa[0].value().get());
    // check if can be merged
    merge_flag_ = target_->size() == 1;
  }

 private:
  bool changed_, merge_flag_;
  BlockSSA *target_;
};

}  // namespace

// register current passs
REGISTER_PASS(BlockMergePass, blk_merge, 1,
              PassStage::PreOpt | PassStage::Opt | PassStage::PostOpt);
