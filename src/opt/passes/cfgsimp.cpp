#include <unordered_set>
#include <memory>

#include "opt/pass.h"
#include "opt/passman.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  this pass will:
  1.  eliminate blocks with only one jump instruction
  2.  replace branch with two equal targets to jump
  3.  replace branch with constant condition to jump

  TODO: eliminate redundant phi node

  e.g.
    %0:
      ...                       %0:
      jump %1                     ...
    %1: ; preds: %0     ==>>      jump %2
      jump %2                   %2: ; preds: %0
    %2: ; preds: %1               ...
      ...

*/
class CFGSimplifyPass : public FunctionPass {
 public:
  CFGSimplifyPass() {}

  bool RunOnFunction(const UserPtr &func) override {
    changed_ = false;
    // traverse all basic blocks
    for (auto it = func->begin(); it != func->end(); ++it) {
      is_entry_ = it == func->begin();
      it->value()->RunPass(*this);
      if (op_ == Op::ReplaceBlock) {
        // replace current block with target
        it->value()->ReplaceBy(target_);
        // remove block from current function
        it->set_value(nullptr);
      }
    }
    if (changed_) func->RemoveValue(nullptr);
    // release value in 'target'
    target_ = nullptr;
    return changed_;
  }

  void RunOn(BlockSSA &ssa) override {
    // check if is jump/branch instruction
    op_ = Op::Nop;
    cur_block_ = &ssa;
    ssa.insts().back()->RunPass(*this);
    switch (op_) {
      case Op::IsJump: {
        if (!is_entry_ && ssa.insts().size() == 1) {
          op_ = Op::ReplaceBlock;
          auto target_block = static_cast<BlockSSA *>(target_.get());
          // get new predecessor set
          std::unordered_set<SSAPtr> preds;
          for (const auto &i : *target_block) {
            if (i.value().get() != &ssa) preds.insert(i.value());
          }
          for (const auto &i : ssa) {
            preds.insert(i.value());
          }
          // apply new predecessors to target block
          target_block->Clear();
          for (const auto &i : preds) {
            target_block->AddValue(i);
          }
          changed_ = true;
        }
        break;
      }
      case Op::ReplaceWithJump: {
        // TODO: check phi nodes in target block
        // create jump instruction
        auto jump = std::make_shared<JumpSSA>(target_);
        jump->set_logger(ssa.insts().back()->logger());
        // replace last instruction with jump
        ssa.insts().back() = jump;
        changed_ = true;
        break;
      }
      default:;
    }
  }

  void RunOn(JumpSSA &ssa) override {
    target_ = ssa[0].value();
    op_ = Op::IsJump;
  }

  void RunOn(BranchSSA &ssa) override {
    if (ssa[1].value() == ssa[2].value()) {
      target_ = ssa[1].value();
      op_ = Op::ReplaceWithJump;
    }
    else if (ssa[0].value()->IsConst()) {
      ssa[0].value()->RunPass(*this);
      target_ = val_ ? ssa[1].value() : ssa[2].value();
      op_ = Op::ReplaceWithJump;
      // remove current block from non-target block's predecessor list
      const auto &block = !val_ ? ssa[1].value() : ssa[2].value();
      auto ptr = static_cast<BlockSSA *>(block.get());
      ptr->RemoveValue(cur_block_);
    }
  }

  void RunOn(ConstIntSSA &ssa) override {
    val_ = ssa.value();
  }

 private:
  enum class Op {
    Nop, IsJump, ReplaceBlock, ReplaceWithJump,
  };

  bool changed_, is_entry_;
  Op op_;
  BlockSSA *cur_block_;
  SSAPtr target_;
  std::uint32_t val_;
};

}  // namespace

// register current passs
REGISTER_PASS(CFGSimplifyPass, cfg_simplify, 1, false);
