#include <unordered_map>
#include <unordered_set>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/helper/ircopier.h"
#include "opt/helper/cast.h"
#include "opt/analysis/loopinfo.h"
#include "opt/helper/const.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  parameters used by pass
*/
// threshold of loop's basic block count
constexpr std::size_t kBlockCountThreshold = 4;
// threshold of loop's trip count
constexpr std::size_t kTripCountThreshold = 80;


// helper pass that performs the actual unrolling operation on the loop
class LoopUnrollerHelperPass : public IRCopier {
 public:
  using IRCopier::RunOn;

  LoopUnrollerHelperPass(const LoopInfo &loop) : loop_(loop) {}

  // perform unrolling, returns false if failed
  bool Unroll() {
    // determine which instruction should be copied
    for (const auto &block : loop_.body) {
      for (const auto &inst : block->insts())  {
        should_copy_.insert(inst.get());
      }
    }
    // copy blocks
    BlockPtr first_entry, last_entry, last_tail;
    for (std::size_t count = 0;; ++count) {
      // check if trip count limit exceeded
      if (count > kTripCountThreshold) {
        // clear all blocks
        for (const auto &b : copied_blocks_) {
          b->ClearInst();
          b->Clear();
        }
        return false;
      }
      // create value for all phi nodes in entry block
      InitCopiedValue(!last_entry);
      // copy entry block
      loop_.entry->RunPass(*this);
      auto entry = SSACast<BlockSSA>(FindCopiedValue(loop_.entry));
      if (!first_entry) first_entry = entry;
      // update predecessor of entry
      if (last_tail) {
        // reroute jump instruction in last tail block
        // its target is 'last_entry', now it should be 'entry'
        auto jump = SSACast<JumpSSA>(last_tail->insts().back().get());
        assert(jump->target() == last_entry);
        jump->set_target(entry);
        // update 'last_tail' and predecessor of entry
        if (entry->size() == 1) {
          auto cur_tail = (*entry)[0].value();
          (*entry)[0].set_value(last_tail);
          last_tail = SSACast<BlockSSA>(cur_tail);
        }
        else {
          assert(entry->empty());
          entry->AddValue(last_tail);
        }
      }
      else if (entry->size() == 1) {
        last_tail = SSACast<BlockSSA>((*entry)[0].value());
        entry->Clear();
      }
      // check if need to continue
      auto jump = SSACast<JumpSSA>(entry->insts().back().get());
      if (jump->target().get() != loop_.exit_block) {
        // update last entry and continue
        last_entry = entry;
      }
      else {
        // unroll completed, stop copying
        ReroutePhiNodes();
        // prevent loop entry from released
        auto loop_entry = loop_.entry->GetPointer();
        // remove blocks of the original loop from parent
        auto parent = loop_.entry->parent();
        for (const auto &block : loop_.body) {
          block->ClearInst();
          if (block != loop_.entry) block->Clear();
          parent->RemoveValue(block);
        }
        // update exit block's predecessors
        loop_.exit_block->RemoveValue(loop_.entry);
        for (const auto &i : loop_.exit_block->insts()) {
          if (auto phi = SSADynCast<PhiSSA>(i.get())) {
            for (const auto &opr : *phi) {
              auto opr_ptr = SSACast<PhiOperandSSA>(opr.value().get());
              if (opr_ptr->block().get() == loop_.entry) {
                opr_ptr->set_block(entry);
                break;
              }
            }
          }
        }
        // add all copied blocks to parent function
        for (const auto &b : copied_blocks_) {
          parent->AddValue(b);
        }
        // reroute jump/branch to original loop entry to unrolled entry
        assert(loop_.entry->size() == 2);
        auto pre = (*loop_.entry)[0].value().get() == loop_.tail
                       ? (*loop_.entry)[1].value()
                       : (*loop_.entry)[0].value();
        loop_.entry->ReplaceBy(first_entry);
        loop_.entry->Clear();
        first_entry->AddValue(pre);
        break;
      }
    }
    return true;
  }

  void RunOn(BlockSSA &ssa) override {
    // do not copy if if block is not in loop
    if (!loop_.body.count(&ssa)) return;
    // create a new block (do not copy predecessors)
    auto block = CopyFromValue(ssa, ssa.parent(), ssa.name());
    copied_blocks_.push_back(block);
    // copy all instructions
    for (const auto &i : ssa.insts()) {
      // copy current instruction
      auto inst = GetCopy(i);
      // skip constant folded instructions
      if (inst->IsConst()) continue;
      // skip phi nodes in entry block
      if (&ssa == loop_.entry && IsSSA<PhiSSA>(i)) continue;
      // insert to block
      block->insts().push_back(inst);
    }
    // handle terminator
    auto &term = block->insts().back();
    if (auto branch = SSADynCast<BranchSSA>(term.get())) {
      UpdatePredecessor(branch->true_block(), block);
      UpdatePredecessor(branch->false_block(), block);
    }
    else {
      UpdatePredecessor(SSACast<JumpSSA>(term.get())->target(), block);
    }
  }

  void RunOn(LoadSSA &ssa) override {
    if (!should_copy_.count(&ssa)) return;
    auto load = CopyFromValue(ssa, nullptr);
    CopyOperand(load, ssa);
  }

  void RunOn(StoreSSA &ssa) override {
    if (!should_copy_.count(&ssa)) return;
    auto store = CopyFromValue(ssa, nullptr, nullptr);
    CopyOperand(store, ssa);
  }

  void RunOn(AccessSSA &ssa) override {
    if (!should_copy_.count(&ssa)) return;
    auto acc = CopyFromValue(ssa, ssa.acc_type(), nullptr, nullptr);
    CopyOperand(acc, ssa);
  }

  void RunOn(BinarySSA &ssa) override {
    if (!should_copy_.count(&ssa)) return;
    auto bin = CopyFromValue(ssa, ssa.op(), nullptr, nullptr);
    CopyOperand(bin, ssa);
    FoldIfPossible(ssa, bin);
  }

  void RunOn(UnarySSA &ssa) override {
    if (!should_copy_.count(&ssa)) return;
    auto una = CopyFromValue(ssa, ssa.op(), nullptr);
    CopyOperand(una, ssa);
    FoldIfPossible(ssa, una);
  }

  void RunOn(CastSSA &ssa) override {
    if (!should_copy_.count(&ssa)) return;
    auto cast = CopyFromValue(ssa, nullptr);
    CopyOperand(cast, ssa);
    FoldIfPossible(ssa, cast);
  }

  void RunOn(CallSSA &ssa) override {
    if (!should_copy_.count(&ssa)) return;
    auto call = CopyFromValue(ssa, nullptr, SSAPtrList(ssa.size() - 1));
    CopyOperand(call, ssa);
  }

  void RunOn(BranchSSA &ssa) override {
    if (!should_copy_.count(&ssa)) return;
    SSAPtr inst;
    const auto &tb = ssa.true_block(), &fb = ssa.false_block();
    // copy condition first
    auto cond = GetCopy(ssa.cond());
    // check if branch can be simplified
    if (auto cint = ConstantHelper::Fold(cond)) {
      // create a jump
      inst = std::make_shared<JumpSSA>(GetCopy(cint->value() ? tb : fb));
    }
    else {
      // create a branch
      inst = std::make_shared<BranchSSA>(cond, GetCopy(tb), GetCopy(fb));
    }
    inst->set_logger(ssa.logger());
    AddCopiedValue(&ssa, inst);
  }

  void RunOn(JumpSSA &ssa) override {
    if (!should_copy_.count(&ssa)) return;
    auto jump = CopyFromValue(ssa, nullptr);
    CopyOperand(jump, ssa);
  }

  void RunOn(ReturnSSA &ssa) override {
    // there shold be no return instructions in the loop
    assert(false);
  }

  void RunOn(AllocaSSA &ssa) override {
    // in fact, alloca instructions should not appear in loop
    // when the IR format is correct
    if (!should_copy_.count(&ssa)) return;
    CopyFromValue(ssa);
  }

  void RunOn(PhiOperandSSA &ssa) override {
    // copy phi operands anyway
    // because this method will be called when copying parent phi node
    auto phi_opr = CopyFromValue(ssa, nullptr, nullptr);
    CopyOperand(phi_opr, ssa);
  }

  void RunOn(PhiSSA &ssa) override {
    if (!should_copy_.count(&ssa)) return;
    auto phi = CopyFromValue(ssa, SSAPtrList(ssa.size()));
    CopyOperand(phi, ssa);
  }

  void RunOn(SelectSSA &ssa) override {
    if (!should_copy_.count(&ssa)) return;
    auto select = CopyFromValue(ssa, nullptr, nullptr, nullptr);
    CopyOperand(select, ssa);
  }

 private:
  // initialize value of phi nodes in loop entry before each iteration
  void InitCopiedValue(bool first_iter) {
    std::unordered_map<Value *, SSAPtr> phi_val;
    // traverse all phi nodes in loop entry
    for (const auto &i : loop_.entry->insts()) {
      if (auto phi = SSADynCast<PhiSSA>(i.get())) {
        for (const auto &o : *phi) {
          auto opr = SSACast<PhiOperandSSA>(o.value().get());
          if (first_iter && opr->block().get() != loop_.tail) {
            // initial value
            phi_val[phi] = opr->value();
            break;
          }
          else if (!first_iter && opr->block().get() == loop_.tail) {
            // value from last iteration
            auto val = FindCopiedValue(opr->value().get());
            assert(val);
            phi_val[phi] = val;
            break;
          }
        }
      }
    }
    // reset copied values
    ClearCopiedValues();
    for (const auto &[phi, val] : phi_val) AddCopiedValue(phi, val);
  }

  // reroute all uses of phi nodes in entry block after unrolling
  void ReroutePhiNodes() {
    for (const auto &i : loop_.entry->insts()) {
      if (IsSSA<PhiSSA>(i.get())) {
        auto val = FindCopiedValue(i.get());
        assert(val);
        i->ReplaceBy(val);
      }
    }
  }

  // fold the specific SSA value if possible
  // if success, replace it with the folded value
  void FoldIfPossible(User &ssa, const SSAPtr &val) {
    if (auto cint = ConstantHelper::Fold(val)) {
      val->ReplaceBy(cint);
      AddCopiedValue(&ssa, cint);
    }
  }

  // update the target block's predecessor
  void UpdatePredecessor(const SSAPtr &target, const SSAPtr &pred) {
    SSACast<BlockSSA>(target.get())->AddValue(pred);
  }

  const LoopInfo &loop_;
  std::unordered_set<Value *> should_copy_;
  std::list<BlockPtr> copied_blocks_;
};


/*
  a naive loop unroller
  this pass will unroll loops with a constant condition

  TODO: this pass must be removed when a new formal loop unroller
        has been implemented
*/
class NaiveLoopUnrollingPass : public FunctionPass {
 public:
  NaiveLoopUnrollingPass() {}

  bool RunOnFunction(const FuncPtr &func) override {
    if (func->is_decl()) return false;
    // run on loops
    const auto &li = PassManager::GetPass<LoopInfoPass>("loop_info");
    const auto &loops = li.GetLoopInfo(func.get());
    for (const auto &loop : loops) {
      if (RunOnLoop(loop)) return true;
    }
    return false;
  }

 private:
  bool CheckLoop(const LoopInfo &loop);
  bool RunOnLoop(const LoopInfo &loop);
};

}  // namespace

// register current pass
REGISTER_PASS(NaiveLoopUnrollingPass, naive_unroll)
    .set_min_opt_level(2)
    .set_stages(PassStage::Opt)
    .Requires("loop_info")
    .Requires("licm")
    .Invalidates("dom_info");


// check if the specific loop can be unrolled
// gather necessary information at the same time
bool NaiveLoopUnrollingPass::CheckLoop(const LoopInfo &loop) {
  // block count of loop can not exceed the threshold
  if (loop.body.size() > kBlockCountThreshold) return false;
  // condition must be a constant comparison
  if (!loop.end_cond) return false;
  if (!loop.end_cond->IsCmp() || (!loop.end_cond->lhs()->IsConst() &&
                                  !loop.end_cond->rhs()->IsConst())) {
    return false;
  }
  // check exit block
  if (!loop.exit_block) return false;
  // check induction variable
  if (!loop.ind_var) return false;
  // initial value must be a constant
  if (!ConstantHelper::Fold(loop.init_val->GetPointer())) return false;
  // modifier must be a binary instruction with a constant operand,
  // another operand must be the induction variable
  if (!loop.modifier) return false;
  if (!((loop.modifier->lhs()->IsConst() &&
         loop.modifier->rhs().get() == loop.ind_var) ||
        (loop.modifier->rhs()->IsConst() &&
         loop.modifier->lhs().get() == loop.ind_var))) {
    return false;
  }
  // this loop can be unrolled
  return true;
}

bool NaiveLoopUnrollingPass::RunOnLoop(const LoopInfo &loop) {
  // check if we can perform unrolling on current loop
  if (!CheckLoop(loop)) return false;
  // perform unrolling
  return LoopUnrollerHelperPass(loop).Unroll();
}
