#include <list>
#include <unordered_map>
#include <algorithm>
#include <cassert>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/passes/helper/cast.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

// break all critical edges in function
class CriticalEdgeBreakerHelperPass : public HelperPass {
 public:
  void BreakOn(const UserPtr &func) {
    // traverse all blocks
    for (const auto &i : *func) {
      i.value()->RunPass(*this);
    }
    // add newly created blocks to function
    for (const auto &i : new_blocks_) {
      i->set_parent(func);
      func->AddValue(i);
    }
  }

  void RunOn(BlockSSA &ssa) override {
    pred_map_.clear();
    // get pointer to current block
    assert(!ssa.uses().empty());
    cur_block_ = ssa.uses().front()->value();
    // traverse all predecessors
    for (auto &&i : ssa) {
      // get terminator of current predecessor
      auto pred = SSACast<BlockSSA>(i.value());
      assert(!pred->insts().empty());
      const auto &terminator = pred->insts().back();
      // check if predecessor has serveral successors
      is_check_ = true;
      has_serveral_succs_ = false;
      terminator->RunPass(*this);
      if (has_serveral_succs_) {
        // create new target block
        auto block = std::make_shared<BlockSSA>(nullptr, "");
        block->AddValue(pred);
        block->set_logger(ssa.logger());
        new_blocks_.push_back(block);
        target_ = block;
        // create jump instruction to current block
        auto jump = std::make_shared<JumpSSA>(cur_block_);
        jump->set_logger(ssa.logger());
        block->AddInst(jump);
        // replace target of branch instruction to new target
        is_check_ = false;
        terminator->RunPass(*this);
        // update predecessor
        pred_map_[pred.get()] = target_;
        i.set_value(target_);
      }
    }
    // traverse all phi nodes to update predecessor
    if (!pred_map_.empty()) {
      is_check_ = true;
      for (const auto &i : ssa.insts()) i->RunPass(*this);
    }
  }

  void RunOn(BranchSSA &ssa) override {
    if (is_check_) {
      // check if current instruction is branch
      assert(ssa[1].value() != ssa[2].value());
      has_serveral_succs_ = true;
    }
    else {
      // replace target block to new target
      auto &use = ssa[1].value() == cur_block_ ? ssa[1] : ssa[2];
      use.set_value(target_);
    }
  }

  void RunOn(PhiSSA &ssa) override {
    // run on all operands
    for (const auto &i : ssa) {
      i.value()->RunPass(*this);
    }
  }

  void RunOn(PhiOperandSSA &ssa) override {
    auto &use = ssa[1];
    // check if need to update
    auto it = pred_map_.find(use.value().get());
    if (it != pred_map_.end()) {
      use.set_value(it->second);
    }
  }

 private:
  bool is_check_, has_serveral_succs_;
  std::list<BlockPtr> new_blocks_;
  SSAPtr cur_block_, target_;
  std::unordered_map<Value *, SSAPtr> pred_map_;
};

// get list of escaped instruction & phi nodes in function
class GetInstsHelperPass : public HelperPass {
 public:
  GetInstsHelperPass(const UserPtr &func) { func->RunPass(*this); }

  void RunOn(FunctionSSA &ssa) override {
    entry_ = SSACast<BlockSSA>(ssa[0].value().get());
    // traverse all blocks to get parent relationship
    for (const auto &use : ssa) {
      auto block = SSACast<BlockSSA>(use.value().get());
      for (const auto &i : block->insts()) {
        parent_map_.insert({i.get(), block});
      }
    }
    // traverse all blocks to get escaped instructions & phi nodes
    for (const auto &use : ssa) {
      auto block = SSACast<BlockSSA>(use.value().get());
      for (const auto &i : block->insts()) {
        ignore_ = is_phi_ = false;
        i->RunPass(*this);
        if (is_phi_) {
          escaped_insts_.push_back(i.get());
          phis_.push_back(SSACast<PhiSSA>(i.get()));
        }
        else if (!ignore_ && IsEscaped(i)) {
          escaped_insts_.push_back(i.get());
        }
      }
    }
  }

  void RunOn(AllocaSSA &ssa) override {
    // ignore all alloca instructions in the entry block
    ignore_ = GetParent(&ssa) == entry_;
  }
  void RunOn(PhiSSA &ssa) override { is_phi_ = true; }

  // get parent block of specific value
  BlockSSA *GetParent(Value *val) {
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

  // getters
  const std::list<Value *> &escaped_insts() const { return escaped_insts_; }
  const std::list<PhiSSA *> &phis() const { return phis_; }

 private:
  bool IsEscaped(const SSAPtr &inst) {
    auto parent = GetParent(inst.get());
    for (const auto &i : inst->uses()) {
      if (GetParent(i->user()) != parent) return true;
    }
    return false;
  }

  bool ignore_, is_phi_;
  BlockSSA *entry_;
  std::list<Value *> escaped_insts_;
  std::list<PhiSSA *> phis_;
  std::unordered_map<Value *, BlockSSA *> parent_map_;
};

/*
  demote all phi nodes to alloca/load/store instructions
  reference:  LLVM, SSA book §3.2
*/
class RegToMemPass : public FunctionPass {
 public:
  RegToMemPass() {}

  bool RunOnFunction(const UserPtr &func) override {
    // skip declarations
    if (func->empty()) return false;
    // find target instructions
    GetInstsHelperPass insts(func);
    if (insts.escaped_insts().empty()) return false;
    insts_ = &insts;
    // try to break critical edges in current function
    CriticalEdgeBreakerHelperPass breaker;
    breaker.BreakOn(func);
    // get entry block
    auto entry = SSACast<BlockSSA>((*func)[0].value());
    assert(entry->empty());
    // demote escaped instructions
    for (const auto &i : insts.escaped_insts()) DemoteRegToStack(i, entry);
    // demote phi nodes
    for (const auto &i : insts.phis()) DemotePhiToStack(i, entry);
    return true;
  }

 private:
  struct InsertPoint {
    BlockPtr block;
    SSAPtrList::iterator pos;
  };

  InsertPoint GetInsertPoint(Value *inst);

  void DemoteRegToStack(Value *inst, const BlockPtr &entry);
  void DemotePhiToStack(PhiSSA *phi, const BlockPtr &entry);

  // reference to helper pass
  GetInstsHelperPass *insts_;
};

}  // namespace

// register current pass
REGISTER_PASS(RegToMemPass, reg2mem, 1, PassStage::Demote);


// get iterator of instruction list of the specific instruction
RegToMemPass::InsertPoint RegToMemPass::GetInsertPoint(Value *inst) {
  // get pointer to parent block
  auto block = insts_->GetParent(inst);
  assert(!block->uses().empty());
  auto block_ptr = SSACast<BlockSSA>(block->uses().front()->value());
  // find current instruction
  SSAPtrList::iterator it;
  for (;;) {
    it = std::find_if(block->insts().begin(), block->insts().end(),
                      [inst](const SSAPtr &ptr) {
                        return ptr.get() == inst;
                      });
    if (it != block->insts().end()) break;
    // 'inst' is a sub-instruction if not found
    assert(inst->uses().size() == 1);
    inst = inst->uses().front()->user();
  }
  return {block_ptr, it};
}

// This function takes a virtual register computed by an instruction and
// replaces it with a slot in the stack frame, allocated via alloca.
// This allows the CFG to be changed around without fear of invalidating
// the SSA information for the value.
void RegToMemPass::DemoteRegToStack(Value *inst, const BlockPtr &entry) {
  if (inst->uses().empty()) return;
  // create an alloca
  auto mod = MakeModule(inst->logger(), entry, entry->insts().begin());
  auto alloca = mod.CreateAlloca(inst->type());
  // change all of the users of the instruction to read from the alloca
  while (!inst->uses().empty()) {
    auto user = inst->uses().front()->user();
    if (auto phi_opr = SSADynCast<PhiOperandSSA>(user)) {
      auto phi = phi_opr->uses().front()->user();
      // insert the load in the predecessor block corresponding to
      // the incoming value
      //
      // NOTE: If there are multiple edges from a basic block to this phi
      // node that we cannot have multiple loads. The problem is that the
      // resulting phi node will have multiple values (from each load)
      // coming in from the same block, which is illegal SSA form.
      // For this reason, we keep track of and reuse loads we insert.
      std::unordered_map<Value *, SSAPtr> loads;
      for (const auto &i : *phi) {
        auto opr = SSACast<PhiOperandSSA>(i.value().get());
        if (opr->value().get() == inst) {
          auto &val = loads[opr->block().get()];
          if (!val) {
            // insert the load into the predecessor block
            auto block = SSACast<BlockSSA>(opr->block());
            mod.SetInsertPoint(block, --block->insts().end());
            val = mod.CreateLoad(alloca);
          }
          opr->set_value(val);
        }
      }
    }
    else {
      // if this is a normal instruction, just insert a load
      auto [block, it] = GetInsertPoint(user);
      mod.SetInsertPoint(block, it);
      auto val = mod.CreateLoad(alloca);
      // reroute all uses to load instruction
      for (auto &&i : *user) {
        if (i.value().get() == inst) i.set_value(val);
      }
    }
  }
  // insert stores of the computed value into the alloca
  auto [block, it] = GetInsertPoint(inst);
  auto inst_ptr = *it++;
  while (IsSSA<PhiSSA>(*it)) ++it;
  mod.SetInsertPoint(block, it);
  mod.CreateStore(inst_ptr, alloca);
}

// This function takes a virtual register computed by a phi node and
// replaces it with a slot in the stack frame allocated via alloca.
// The phi node is deleted.
void RegToMemPass::DemotePhiToStack(PhiSSA *phi, const BlockPtr &entry) {
  if (phi->uses().empty()) return;
  // create an alloca
  auto mod = MakeModule(phi->logger(), entry, entry->insts().begin());
  auto alloca = mod.CreateAlloca(phi->type());
  // iterate over each operand inserting a store in each predecessor
  for (const auto &i : *phi) {
    auto opr = SSACast<PhiOperandSSA>(i.value().get());
    auto block = SSACast<BlockSSA>(opr->block());
    mod.SetInsertPoint(block, --block->insts().end());
    mod.CreateStore(opr->value(), alloca);
  }
  // get insert point of phi node and erase
  auto [block, it] = GetInsertPoint(phi);
  it = block->insts().erase(it);
  // skip phi nodes after current phi
  while (IsSSA<PhiSSA>(*it)) ++it;
  // insert a load and replace all uses
  mod.SetInsertPoint(block, it);
  auto val = mod.CreateLoad(alloca);
  phi->ReplaceBy(val);
}
