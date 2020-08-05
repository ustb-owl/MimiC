#include <unordered_map>
#include <unordered_set>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/passes/helper/ircopier.h"
#include "opt/passes/helper/cast.h"
#include "opt/passes/helper/loop.h"
#include "opt/passes/helper/const.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  parameters used by pass
*/
// threshold of loop's basic block count
constexpr std::size_t kBlockCountThreshold = 4;


// information required by loop unroller
struct UnrollInfo {
  // indicates if current loop can be unrolled
  bool can_unroll;
  // induction variable
  PhiSSA *ind_var;
  // initial value of induction variable
  std::uint32_t init_val;
  // instruction to modify the induction variable in each loop
  BinarySSA *modifier;
  // the end condition of loop
  BinarySSA *end_cond;
  // block to jump to when the loop exits
  BlockSSA *exit_block;
};


// helper pass that performs the actual unrolling operation on the loop
class LoopUnrollerHelperPass : public IRCopier {
 public:
  using IRCopier::RunOn;

  LoopUnrollerHelperPass(const LoopInfo &loop, const UnrollInfo &ui)
      : loop_(loop), ui_(ui) {}

  void Unroll() {
    assert(ui_.can_unroll);
    // determine which instruction should be copied
    for (const auto &block : loop_.body) {
      for (const auto &inst : block->insts())  {
        should_copy_.insert(inst.get());
      }
    }
    // copy blocks
    BlockPtr first_entry, last_entry, last_tail;
    for (;;) {
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
      if (jump->target().get() != ui_.exit_block) {
        // update last entry and continue
        last_entry = entry;
      }
      else {
        // unroll completed, stop copying
        ReroutePhiNodes(last_entry != nullptr);
        // remove blocks of the original loop from parent
        assert(!loop_.entry->uses().empty());
        // prevent loop entry from released
        auto loop_entry = loop_.entry->uses().front()->value();
        auto parent = loop_.entry->parent();
        for (const auto &block : loop_.body) {
          block->insts().clear();
          if (block != loop_.entry) block->Clear();
          parent->RemoveValue(block);
        }
        ui_.exit_block->RemoveValue(loop_.entry);
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
  }

  void RunOn(BlockSSA &ssa) override {
    // do not copy if if block is not in loop
    if (!loop_.body.count(&ssa)) return;
    // create a new block (do not copy predecessors)
    auto block = CopyFromValue(ssa, ssa.parent(), ssa.name());
    ssa.parent()->AddValue(block);
    // copy all instructions
    for (const auto &i : ssa.insts()) {
      // copy current instruction
      auto inst = GetCopy(i);
      // skip constant folded instructions
      if (inst->IsConst()) continue;
      // skip phi nodes in entry block
      if (&ssa == loop_.entry && IsSSA<PhiSSA>(inst)) continue;
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
  void ReroutePhiNodes(bool unrolled) {
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
  const UnrollInfo &ui_;
  std::unordered_set<Value *> should_copy_;
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

  bool RunOnFunction(const UserPtr &func) override {
    auto func_ptr = SSACast<FunctionSSA>(func.get());
    if (func_ptr->is_decl()) return false;
    // run on loops
    bool changed = false;
    LoopDetector ld(func_ptr);
    for (const auto &loop : ld.loops()) {
      if (RunOnLoop(loop)) {
        changed = true;
      }
    }
    return changed;
  }

 private:
  UnrollInfo CheckLoop(const LoopInfo &loop);
  bool RunOnLoop(const LoopInfo &loop);
};

}  // namespace

// register current pass
REGISTER_PASS(NaiveLoopUnrollingPass, naive_unroll, 2, PassStage::Opt);


// check if the specific loop can be unrolled
// gather necessary information at the same time
UnrollInfo NaiveLoopUnrollingPass::CheckLoop(const LoopInfo &loop) {
  UnrollInfo ui = {false};
  // block count of loop can not exceed the threshold
  if (loop.body.size() > kBlockCountThreshold) return ui;
  // the entry of loop must have only two predecessors
  if (loop.entry->size() != 2) return ui;
  // loop must have only one exit block, and it must be entry block
  if (loop.exit.size() != 1 || !loop.exit.count(loop.entry)) return ui;
  // get condition
  auto branch = SSACast<BranchSSA>(loop.entry->insts().back().get());
  if (auto bin = SSADynCast<BinarySSA>(branch->cond().get())) {
    // condition must be a constant comparison
    if (!bin->IsCmp() ||
        (!bin->lhs()->IsConst() && !bin->rhs()->IsConst())) {
      return ui;
    }
    ui.end_cond = bin;
  }
  else {
    // condition must be a binary instruction
    return ui;
  }
  // get exit block
  auto tb = SSACast<BlockSSA>(branch->true_block().get());
  auto fb = SSACast<BlockSSA>(branch->false_block().get());
  ui.exit_block = loop.body.count(tb) ? fb : tb;
  // check induction variable
  auto ind_var = ui.end_cond->lhs()->IsConst() ? ui.end_cond->rhs().get()
                                               : ui.end_cond->lhs().get();
  if (auto phi = SSADynCast<PhiSSA>(ind_var)) {
    assert(phi->size() == 2);
    ui.ind_var = phi;
    // check for operands
    for (const auto &i : *phi) {
      auto opr = SSACast<PhiOperandSSA>(i.value().get());
      if (opr->block().get() != loop.tail) {
        // initial value must be a constant
        if (auto cint = ConstantHelper::Fold(opr->value())) {
          ui.init_val = cint->value();
        }
        else {
          return ui;
        }
      }
      else {
        // modifier must be a binary instruction with a constant operand,
        // another operand must be the induction variable
        if (auto bin = SSADynCast<BinarySSA>(opr->value().get())) {
          if (!((bin->lhs()->IsConst() && bin->rhs().get() == phi) ||
                (bin->rhs()->IsConst() && bin->lhs().get() == phi))) {
            return ui;
          }
          ui.modifier = bin;
        }
        else {
          return ui;
        }
      }
    }
  }
  else {
    // induction variable must be a phi node
    return ui;
  }
  // this loop can be unrolled
  ui.can_unroll = true;
  return ui;
}

bool NaiveLoopUnrollingPass::RunOnLoop(const LoopInfo &loop) {
  // check if we can perform unrolling on current loop
  auto ui = CheckLoop(loop);
  if (!ui.can_unroll) return false;
  // perform unroll
  LoopUnrollerHelperPass(loop, ui).Unroll();
  return true;
}
