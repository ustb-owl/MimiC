#include <unordered_set>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/passes/helper/cast.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

// helper pass for block merging
// check if blocks can be merged, if so, merge predecessors of two block
// merge can not be done if target block contains any phi nodes
class BlockMergeHelperPass : public HelperPass {
 public:
  bool CheckAndMerge(BlockSSA *cur, BlockSSA *target) {
    // check if target contains phi node
    is_phi_ = false;
    for (const auto &i : target->insts()) {
      i->RunPass(*this);
      if (is_phi_) return false;
    }
    // get new predecessor set
    std::unordered_set<SSAPtr> preds;
    for (const auto &i : *target) {
      if (i.value().get() != cur) preds.insert(i.value());
    }
    for (const auto &i : *cur) {
      preds.insert(i.value());
    }
    // apply new predecessors to target block
    target->Clear();
    for (const auto &i : preds) target->AddValue(i);
    return true;
  }

  void RunOn(PhiSSA &ssa) override { is_phi_ = true; }

 private:
  bool is_phi_;
};

// helper pass for dead block elimination
// remove specific block from other blocks' preds & phi nodes
class BlockElimHelperPass : public HelperPass {
 public:
  BlockElimHelperPass(BlockSSA *block) : block_(block) {}

  void RunOn(BlockSSA &ssa) override {
    // remove from predecessors
    ssa.RemoveValue(block_);
  }

  void RunOn(PhiOperandSSA &ssa) override {
    // check if parent block is target block
    if (ssa.block().get() == block_) {
      ssa.RemoveFromUser();
      // simplify parent phi node if necessary
      auto phi = ssa.uses().front()->user();
      if (phi->size() == 1) phi->ReplaceBy(ssa.value());
    }
  }

 private:
  BlockSSA *block_;
};

/*
  simplify CFG
  this pass will:
  1.  eliminate dead blocks
  2.  eliminate blocks with only one jump instruction (partly)

  TODO:
  1.  eliminate blocks with only one jump instruction (fully)
  2.  simplify redundant phi nodes by inserting select instructions

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
    // traverse all basic blocks except entry
    for (std::size_t i = 1; i < func->size(); ++i) {
      auto &it = (*func)[i];
      it.value()->RunPass(*this);
      // check if need to merge (replace) block
      if (op_ == Op::ReplaceBlock) {
        // replace current block with target
        it.value()->ReplaceBy(target_);
        // mark current block as removed
        it.set_value(nullptr);
      }
    }
    // remove all marked blocks
    if (changed_) func->RemoveValue(nullptr);
    // release value in 'target'
    target_ = nullptr;
    return changed_;
  }

  void RunOn(BlockSSA &ssa) override {
    op_ = Op::Nop;
    if (ssa.insts().size() == 1) {
      // block contains only one instruction
      ssa.insts().back()->RunPass(*this);
      // check if is jump instruction
      if (op_ == Op::IsJump) {
        // check if can be merged
        BlockMergeHelperPass helper;
        auto target = SSACast<BlockSSA>(target_.get());
        if (helper.CheckAndMerge(&ssa, target)) {
          op_ = Op::ReplaceBlock;
          changed_ = true;
        }
      }
    }
    else if (ssa.empty()) {
      // block has no predecessor
      if (ssa.insts().size() > 1) {
        ssa.logger()->LogWarning("unreachable code");
      }
      // remove current block from all users except parent function
      BlockElimHelperPass helper(&ssa);
      auto uses = ssa.uses();
      for (const auto &i : uses) i->user()->RunPass(helper);
      // mark current block as removed
      ssa.ReplaceBy(nullptr);
      changed_ = true;
    }
  }

  void RunOn(JumpSSA &ssa) override {
    target_ = ssa.target();
    op_ = Op::IsJump;
  }

 private:
  enum class Op {
    Nop, IsJump, ReplaceBlock,
  };

  bool changed_;
  Op op_;
  SSAPtr target_;
};

}  // namespace

// register current passs
REGISTER_PASS(CFGSimplifyPass, cfg_simplify, 1,
              PassStage::PreOpt | PassStage::Opt | PassStage::PostOpt);
