#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <queue>
#include <list>
#include <cstddef>
#include <cassert>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/helper/cast.h"
#include "opt/helper/const.h"
#include "opt/helper/inst.h"
#include "mid/module.h"
#include "utils/hashing.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

// type of lattice value
enum class LatticeType { Unknown, Overdefined, Const, ForcedConst };

// representation of a lattice value
// which can be unknown (undefined), overdefined
// or a certain constant integer (const or forced const)
class LatticeVal {
 public:
  // default to unknown value
  LatticeVal() : type_(LatticeType::Unknown) {}

  // set as overdefined value
  // returns false if does not change
  bool setAsOverdefined() {
    if (type_ == LatticeType::Overdefined) return false;
    type_ = LatticeType::Overdefined;
    value_ = nullptr;
    return true;
  }

  // set as a constant value
  // returns false if does not change
  bool setAsConst(const SSAPtr &value) {
    assert(value && IsSSA<ConstIntSSA>(value));
    // has already been a constant but not a forced constant
    if (type_ == LatticeType::Const) {
      assert(**this == SSACast<ConstIntSSA>(value.get())->value());
      return false;
    }
    // check if is unknown before
    if (type_ == LatticeType::Unknown) {
      // just set as constant
      type_ = LatticeType::Const;
      value_ = value;
    }
    else {
      // must be forced constant (overdefined -> constant is forbidden)
      assert(type_ == LatticeType::ForcedConst);
      // stay at forced constant if value not changed
      if (**this == SSACast<ConstIntSSA>(value.get())->value()) {
        return false;
      }
      // otherwise, set as overdefined
      // because this forced constant are possibly wrong
      type_ = LatticeType::Overdefined;
      value_ = nullptr;
    }
    return true;
  }

  // set as a forced constant value
  void setAsForcedConst(const SSAPtr &value) {
    assert(value && IsSSA<ConstIntSSA>(value));
    type_ = LatticeType::ForcedConst;
    value_ = value;
  }

  // return true if current lattice is a constant
  operator bool() const { return type_ == LatticeType::Const; }
  // return value of constant integer
  std::uint32_t operator*() const {
    assert(*this && value_);
    return SSACast<ConstIntSSA>(value_.get())->value();
  }

  // getters
  bool is_unknown() const { return type_ == LatticeType::Unknown; }
  bool is_overdefined() const { return type_ == LatticeType::Overdefined; }
  bool is_const() const {
    return type_ == LatticeType::Const || type_ == LatticeType::ForcedConst;
  }
  const SSAPtr &value() const { return value_; }

 private:
  LatticeType type_;
  SSAPtr value_;
};

/*
  sparse conditional constant propagation
  this pass will perform SCCP on function

  reference:  Constant Propagation with Conditional Branches
              LLVM
*/
class SparseCondConstPropagationPass : public FunctionPass {
 public:
  SparseCondConstPropagationPass() {}

  bool RunOnFunction(const FuncPtr &func) override {
    if (func->is_decl()) return false;
    // scan for parent info
    ParentScanner parent(func);
    parent_ = &parent;
    // mark entry block as executable
    auto entry = SSACast<BlockSSA>(func->entry().get());
    MarkBlockExecutable(entry);
    // mark all arguments as overdefined
    for (const auto &i : func->args()) {
      MarkOverdefined(i.get());
    }
    // solve for constants
    bool resoved_undefs = true;
    while (resoved_undefs) {
      Solve(func.get());
      resoved_undefs = ResolvedUndefsIn(func.get());
    }
    // traverse all blocks & apply changes
    bool changed = false;
    for (const auto &i : *func) {
      auto block = SSACast<BlockSSA>(i.value().get());
      auto &insts = block->insts();
      if (!IsBlockExecutable(block)) {
        // clear all instructions (except terminator) in current block
        auto mod = MakeModule(nullptr);
        while (insts.size() > 1) {
          auto &inst = insts.front();
          if (!inst->uses().empty() && inst->type() &&
              !inst->type()->IsVoid()) {
            // replace with undef
            auto log = mod.SetContext(insts.front()->logger());
            insts.front()->ReplaceBy(mod.GetUndef(insts.front()->type()));
          }
          // remove from list
          insts.pop_front();
          if (!changed) changed = true;
        }
      }
      else {
        // traverse all instruction & replace with constants
        for (auto it = insts.begin(); it != insts.end();) {
          const auto &inst = *it;
          if (TryToReplaceWithConst(inst.get())) {
            assert(!IsSSA<CallSSA>(inst));
            // remove from block
            it = insts.erase(it);
            if (!changed) changed = true;
          }
          else {
            ++it;
          }
        }
      }
    }
    return false;
  }

  void CleanUp() override {
    values_.clear();
    exe_blocks_.clear();
    known_edges_.clear();
    assert(block_list_.empty() && inst_list_.empty() &&
           overdefed_list_.empty());
  }

  void RunOn(LoadSSA &ssa) override;
  void RunOn(StoreSSA &ssa) override;
  void RunOn(AccessSSA &ssa) override;
  void RunOn(BinarySSA &ssa) override;
  void RunOn(UnarySSA &ssa) override;
  void RunOn(CastSSA &ssa) override;
  void RunOn(CallSSA &ssa) override;
  void RunOn(BranchSSA &ssa) override;
  void RunOn(JumpSSA &ssa) override;
  void RunOn(ReturnSSA &ssa) override;
  void RunOn(AllocaSSA &ssa) override;
  void RunOn(BlockSSA &ssa) override;
  void RunOn(ArgRefSSA &ssa) override;
  void RunOn(PhiOperandSSA &ssa) override;
  void RunOn(PhiSSA &ssa) override;
  void RunOn(SelectSSA &ssa) override;

 private:
  // mark block as executable
  // returns false if block has already been executable
  bool MarkBlockExecutable(BlockSSA *block) {
    if (!exe_blocks_.insert(block).second) return false;
    block_list_.push(block);
    return true;
  }

  // check if block is executable
  bool IsBlockExecutable(BlockSSA *block) {
    return exe_blocks_.count(block);
  }

  // mark an edge of basic block as executable
  bool MarkEdgeExecutable(BlockSSA *source, BlockSSA *dest) {
    // current edge is already known to be executable
    if (!known_edges_.insert({source, dest}).second) return false;
    if (!MarkBlockExecutable(dest)) {
      // If the destination is already executable, we just made
      // an edge feasible that wasn't before. Revisit the phi node
      // in the block because they have potentially new operands
      for (const auto &i : dest->insts()) {
        if (auto phi = SSADynCast<PhiSSA>(i.get())) phi->RunPass(*this);
      }
    }
    return true;
  }

  // check if the specific edge is currently feasible
  bool IsEdgeFeasible(BlockSSA *from, BlockSSA *to) {
    return known_edges_.count({from, to});
  }

  // mark all users of value as changed
  void MarkUsersAsChanged(Value *val) {
    for (const auto &i : val->uses()) {
      if (IsBlockExecutable(parent_->GetParent(i->user()))) {
        i->user()->RunPass(*this);
      }
    }
  }

  // return the lattice value of the specific value
  // set as constant value if parameter is a constant
  LatticeVal &GetValue(const SSAPtr &val) {
    auto [it, succ] = values_.insert({val.get(), LatticeVal()});
    if (succ) {
      if (auto cint = ConstantHelper::Fold(val)) {
        it->second.setAsConst(cint);
      }
    }
    return it->second;
  }

  // push value to worklist
  void PushToWorklist(const LatticeVal &lv, Value *val) {
    if (lv.is_overdefined()) {
      overdefed_list_.push(val);
    }
    else {
      inst_list_.push(val);
    }
  }

  // mark a value as forced constant, and push it to worklist
  void MarkForcedConst(Value *val, const SSAPtr &c) {
    auto &lv = values_[val];
    lv.setAsForcedConst(c);
    PushToWorklist(lv, val);
  }

  // mark a value as overdefined, and push it to worklist
  void MarkOverdefined(LatticeVal &lv, Value *val) {
    if (!lv.setAsOverdefined()) return;
    PushToWorklist(lv, val);
  }
  void MarkOverdefined(Value *val) {
    MarkOverdefined(values_[val], val);
  }
  
  // mark a value as constant
  void MarkConst(LatticeVal &lv, Value *val, const SSAPtr &c) {
    if (!lv.setAsConst(c)) return;
    PushToWorklist(lv, val);
  }
  void MarkConst(Value *val, const SSAPtr &c) {
    MarkConst(values_[val], val, c);
  }

  // merge another lattice value in current lattice value
  void MergeInValue(Value *val, const LatticeVal &rhs) {
    auto &lv = values_[val];
    if (lv.is_overdefined() || rhs.is_unknown()) return;
    if (rhs.is_overdefined()) return MarkOverdefined(lv, val);
    if (lv.is_unknown()) return MarkConst(lv, val, rhs.value());
    if (*lv != *rhs) return MarkOverdefined(lv, val);
  }

  void Solve(FunctionSSA *func);
  bool ResolvedUndefsIn(FunctionSSA *func);
  bool TryToReplaceWithConst(Value *val);

  // helper class for scanning parent
  ParentScanner *parent_;
  // all evaluated values
  std::unordered_map<Value *, LatticeVal> values_;
  // all executable blocks
  std::unordered_set<BlockSSA *> exe_blocks_;
  // edges which have already had phi nodes retriggered
  std::unordered_set<std::pair<BlockSSA *, BlockSSA *>> known_edges_;
  // work list for blocks
  std::queue<BlockSSA *> block_list_;
  // work list for variables
  std::queue<Value *> inst_list_, overdefed_list_;
};

}  // namespace

// register current pass
REGISTER_PASS(SparseCondConstPropagationPass, sccp)
    .set_min_opt_level(1)
    .set_stages(PassStage::Opt);


/*
==================== visitor methods begin =====================
Something changed in this instruction, either an operand made a
transition, or the instruction is newly executable. Change the
value type of instruction to reflect these changes if appropriate.

These methods makes sure to do the following actions:
1.  If a phi node merges two constants in, and has conflicting
    value coming from different branches, or if the phi node merges
    in an overdefined value, then the phi node becomes overdefined.
2.  If a phi node merges only constants in, and they all agree on
    value, the PHI node becomes a constant value equal to that.
3.  If V <- x (op) y && isConstant(x) && isConstant(y), V = Constant
4.  If V <- x (op) y && (isOverdefined(x) || isOverdefined(y)),
    V = Overdefined
5.  If V <- MEM or V <- CALL or V <- (unknown) then V = Overdefined
6.  If a conditional branch has a value that is constant, make the
    selected destination executable
7.  If a conditional branch has a value that is overdefined, make
    all successors executable.
*/

void SparseCondConstPropagationPass::RunOn(LoadSSA &ssa) {
  auto &ssa_val = values_[&ssa];
  if (ssa_val.is_overdefined()) return;
  // if is not a load of an integer, just mark as overdefined
  if (!ssa.type()->IsInteger()) return MarkOverdefined(ssa_val, &ssa);
  // check if is loading a constant global (or it's pointer)
  if (auto gvar = SSADynCast<GlobalVarSSA>(ssa.ptr().get())) {
    if (gvar->is_var()) return MarkOverdefined(ssa_val, &ssa);
    // loading from constant global variable
    SSAPtr val;
    if (gvar->init()) {
      val = gvar->init();
    }
    else {
      val = MakeModule(gvar->logger()).GetInt(0, ssa.type());
    }
    return MarkConst(ssa_val, &ssa, val);
  }
  // otherwise, treat as overdefined
  MarkOverdefined(ssa_val, &ssa);
}

void SparseCondConstPropagationPass::RunOn(StoreSSA &ssa) {
  // store instruction is not a value
}

void SparseCondConstPropagationPass::RunOn(AccessSSA &ssa) {
  using AccType = AccessSSA::AccessType;
  // if all operands are constants then we can turn this into a constant
  if (values_[&ssa].is_overdefined()) return;
  // mark access from global variables as overdefined
  if (IsSSA<GlobalVarSSA>(ssa.ptr())) return MarkOverdefined(&ssa);
  // not resolved yet
  const auto &ptr = GetValue(ssa.ptr()), &index = GetValue(ssa.index());
  if (ptr.is_unknown() || index.is_unknown()) return;
  // not a constant
  if (ptr.is_overdefined() || index.is_overdefined()) {
    return MarkOverdefined(&ssa);
  }
  // perform address calculation
  std::uint32_t val;
  if (ssa.acc_type() == AccType::Pointer ||
      (ssa.acc_type() == AccType::Element && ssa.type()->IsArray())) {
    val = *ptr + *index * ssa.type()->GetDerefedType()->GetSize();
  }
  else {
    assert(ssa.acc_type() == AccType::Element && ssa.type()->IsStruct());
    val = *ptr;
    // calculate offset
    auto base_ty = ssa.type()->GetDerefedType();
    auto align = base_ty->GetAlignSize();
    for (std::size_t i = 0; i < *index; ++i) {
      auto k = (base_ty->GetElem(i)->GetSize() + align - 1) / align;
      val += align * k;
    }
  }
  // mark as constant
  auto mod = MakeModule(ssa.logger());
  MarkConst(&ssa, mod.GetInt(val, ssa.type()));
}

void SparseCondConstPropagationPass::RunOn(BinarySSA &ssa) {
  using Op = BinarySSA::Operator;
  const auto &lhs = GetValue(ssa.lhs()), &rhs = GetValue(ssa.rhs());
  auto &ssa_val = values_[&ssa];
  if (ssa_val.is_overdefined()) return;
  // fold if is constant
  if (lhs && rhs) {
    auto cint = ConstantHelper::Fold(ssa.op(), lhs.value(), rhs.value());
    return MarkConst(ssa_val, &ssa, cint);
  }
  // if something is undef, wait for it to resolve
  if (!lhs.is_overdefined() && !rhs.is_overdefined()) return;
  // otherwise, try to produce something better
  // 0 / X -> 0
  if ((ssa.op() == Op::UDiv || ssa.op() == Op::SDiv) && lhs && !*lhs) {
    return MarkConst(ssa_val, &ssa, lhs.value());
  }
  // and/mul with 0 -> 0
  // or with -1 -> -1
  if (ssa.op() == Op::And || ssa.op() == Op::Mul || ssa.op() == Op::Or) {
    const LatticeVal *non_overdef = nullptr;
    if (!lhs.is_overdefined()) {
      non_overdef = &lhs;
    }
    else if (!lhs.is_overdefined()) {
      non_overdef = &rhs;
    }
    if (non_overdef) {
      if (non_overdef->is_unknown()) return;
      if (ssa.op() == Op::And || ssa.op() == Op::Mul) {
        // X & 0 -> 0
        // X * 0 -> 0
        if (!**non_overdef) {
          return MarkConst(ssa_val, &ssa, non_overdef->value());
        }
      }
      else {
        // X | -1 -> -1
        if (**non_overdef == static_cast<std::uint32_t>(-1)) {
          return MarkConst(ssa_val, &ssa, non_overdef->value());
        }
      }
    }
  }
  MarkOverdefined(&ssa);
}

void SparseCondConstPropagationPass::RunOn(UnarySSA &ssa) {
  const auto &opr = GetValue(ssa.opr());
  auto &ssa_val = values_[&ssa];
  if (ssa_val.is_overdefined()) return;
  // fold if is constant
  if (opr) {
    auto cint = ConstantHelper::Fold(ssa.op(), opr.value());
    return MarkConst(ssa_val, &ssa, cint);
  }
  // if something is undef, wait for it to resolve
  if (!opr.is_overdefined()) return;
  MarkOverdefined(&ssa);
}

void SparseCondConstPropagationPass::RunOn(CastSSA &ssa) {
  const auto &lv = GetValue(ssa.opr());
  // inherit overdefinedness of operand
  if (lv.is_overdefined()) return MarkOverdefined(&ssa);
  // mark access from global variables as overdefined
  if (IsSSA<GlobalVarSSA>(ssa.opr())) return MarkOverdefined(&ssa);
  if (lv) {
    MarkConst(&ssa, ConstantHelper::Fold(lv.value(), ssa.type()));
  }
}

void SparseCondConstPropagationPass::RunOn(CallSSA &ssa) {
  // treat all function call as overdefined
  MarkOverdefined(&ssa);
}

void SparseCondConstPropagationPass::RunOn(BranchSSA &ssa) {
  const auto &cond = GetValue(ssa.cond());
  auto block = parent_->GetParent(&ssa);
  auto tb = SSACast<BlockSSA>(ssa.true_block().get());
  auto fb = SSACast<BlockSSA>(ssa.false_block().get());
  // no successor is executable when condition is unknown
  if (cond.is_unknown()) return;
  // all successor is executable when condition is overdefined
  if (cond.is_overdefined()) {
    MarkEdgeExecutable(block, tb);
    MarkEdgeExecutable(block, fb);
    return;
  }
  // constant condition, branch can only go a single way
  MarkEdgeExecutable(block, *cond ? tb : fb);
}

void SparseCondConstPropagationPass::RunOn(JumpSSA &ssa) {
  // mark the edge to target as executable
  auto block = parent_->GetParent(&ssa);
  auto target = SSACast<BlockSSA>(ssa.target().get());
  MarkEdgeExecutable(block, target);
}

void SparseCondConstPropagationPass::RunOn(ReturnSSA &ssa) {
  // return instruction is not a value
}

void SparseCondConstPropagationPass::RunOn(AllocaSSA &ssa) {
  // just mark as overdefined
  MarkOverdefined(&ssa);
}

void SparseCondConstPropagationPass::RunOn(BlockSSA &ssa) {
  // visit all instructions
  for (const auto &i : ssa.insts()) {
    i->RunPass(*this);
  }
}

void SparseCondConstPropagationPass::RunOn(ArgRefSSA &ssa) {
  // just mark as overdefined
  MarkOverdefined(&ssa);
}

void SparseCondConstPropagationPass::RunOn(PhiOperandSSA &ssa) {
  // tell it's parent phi node
  assert(ssa.uses().size() == 1);
  auto phi = ssa.uses().front()->user();
  phi->RunPass(*this);
}

void SparseCondConstPropagationPass::RunOn(PhiSSA &ssa) {
  // quick exit
  if (values_[&ssa].is_overdefined()) return;
  // super-extra-high-degree phi nodes are unlikely to ever be
  // marked constant, skip for speed
  if (ssa.size() > 64) return MarkOverdefined(&ssa);
  // look at all of the executable operands of the phi node, if:
  // 1. any of them are overdefined, phi becomes overdefined as well
  // 2. they are all constant and agree with each other, phi becomes
  //    the identical constant
  // 3. they are constant and don't agree, phi is overdefined
  // 4. there are no executable operands, phi remains unknown
  SSAPtr opr_val;
  for (const auto &i : ssa) {
    auto opr = SSACast<PhiOperandSSA>(i.value().get());
    const auto &lv = GetValue(opr->value());
    // doesn't influence phi node
    if (lv.is_unknown()) continue;
    // check if pred is not executable
    auto from = SSACast<BlockSSA>(opr->block().get());
    if (!IsEdgeFeasible(from, parent_->GetParent(&ssa))) continue;
    // mark current phi as overdefined
    if (lv.is_overdefined()) return MarkOverdefined(&ssa);
    // grab the first value
    if (!opr_val) {
      opr_val = lv.value();
      continue;
    }
    // two conflicted constants, mark as overdefined
    if ((IsSSA<ConstZeroSSA>(opr_val) && *lv) ||
        (SSACast<ConstIntSSA>(opr_val.get())->value() != *lv)) {
      return MarkOverdefined(&ssa);
    }
  }
  // just mark as constant if not unknown
  if (opr_val) MarkConst(&ssa, opr_val);
}

void SparseCondConstPropagationPass::RunOn(SelectSSA &ssa) {
  const auto &cond = GetValue(ssa.cond());
  if (cond.is_unknown()) return;
  // fold if condition is constant
  if (cond) {
    auto opr = *cond ? ssa.true_val() : ssa.false_val();
    MergeInValue(&ssa, GetValue(opr));
    return;
  }
  // otherwise, the condition is overdefined or a constant we can't eval
  // see if we can produce something better that overdefined
  const auto &tv = GetValue(ssa.true_val());
  const auto &fv = GetValue(ssa.false_val());
  // select ?, val, val -> val
  if (tv && fv && *tv == *fv) return MarkConst(&ssa, tv.value());
  // select ?, undef, X -> X
  if (tv.is_unknown()) return MergeInValue(&ssa, fv);
  // select ?, X, undef -> X
  if (fv.is_unknown()) return MergeInValue(&ssa, tv);
  // other situations
  MarkOverdefined(&ssa);
}

/*
==================== end of visitor methods ====================
*/


// solve for constants and executable blocks
void SparseCondConstPropagationPass::Solve(FunctionSSA *func) {
  // process until all work lists are empty
  while (!block_list_.empty() || !inst_list_.empty() ||
         !overdefed_list_.empty()) {
    // handle overdefined value first for speed
    while (!overdefed_list_.empty()) {
      auto val = overdefed_list_.front();
      overdefed_list_.pop();
      MarkUsersAsChanged(val);
    }
    // handle other instructions
    while (!inst_list_.empty()) {
      auto val = inst_list_.front();
      inst_list_.pop();
      MarkUsersAsChanged(val);
    }
    // handle blocks
    while (!block_list_.empty()) {
      auto block = block_list_.front();
      block_list_.pop();
      block->RunPass(*this);
    }
  }
}

// this method will:
// 1. handle branchs with undefined condition
// 2. check for values that use undefs, whose results are actually defined
// returns true when solver should be rerun
bool SparseCondConstPropagationPass::ResolvedUndefsIn(FunctionSSA *func) {
  for (const auto &use : *func) {
    auto block = SSACast<BlockSSA>(use.value().get());
    if (!IsBlockExecutable(block)) continue;
    // check for all instructions
    for (const auto &i : block->insts()) {
      // look for instructions which produce undef values
      if (!i->type() ||
          !(i->type()->IsInteger() && i->type()->IsPointer())) {
        continue;
      }
      auto &lv = GetValue(i);
      if (!lv.is_unknown()) continue;
      // handle by SSA type
      if (auto ssa = SSADynCast<BinarySSA>(i.get())) {
        using Op = BinarySSA::Operator;
        // make a temp module
        auto mod = MakeModule(ssa->logger());
        // get operands
        const auto &lhs = GetValue(ssa->lhs());
        const auto &rhs = GetValue(ssa->rhs());
        switch (ssa->op()) {
          case Op::Add: case Op::Sub: {
            // any undef -> undef
            break;
          }
          case Op::Mul: case Op::And: {
            // undef op undef -> undef
            if (lhs.is_unknown() && rhs.is_unknown()) break;
            // undef op X -> 0, X could be zero
            MarkForcedConst(ssa, mod.GetInt(0, ssa->type()));
            return true;
          }
          case Op::Or: {
            // undef | undef -> undef
            if (lhs.is_unknown() && rhs.is_unknown()) break;
            // undef | X -> -1, X could be -1
            MarkForcedConst(ssa, mod.GetInt(-1, ssa->type()));
            return true;
          }
          case Op::Xor: {
            // undef ^ undef -> 0
            if (lhs.is_unknown() && rhs.is_unknown()) {
              MarkForcedConst(ssa, mod.GetInt(0, ssa->type()));
              return true;
            }
            // undef ^ X -> undef
            break;
          }
          case Op::UDiv: case Op::SDiv: case Op::URem: case Op::SRem: {
            // X op undef -> undef
            if (rhs.is_unknown()) break;
            // X op 0 -> undef
            if (rhs && !*rhs) break;
            // undef / X -> 0, X cound be maxint
            // undef % X -> 0, X cound be 1
            MarkForcedConst(ssa, mod.GetInt(0, ssa->type()));
            return true;
          }
          case Op::Shl: case Op::LShr: case Op::AShr: {
            // X op undef -> undef
            if (rhs.is_unknown()) break;
            // shifting by the bitwidth or more is undefined
            if (rhs && *rhs >= ssa->type()->GetSize() * 8) break;
            // undef op X -> 0
            MarkForcedConst(ssa, mod.GetInt(0, ssa->type()));
            return true;
          }
          case Op::Equal: {
            // X == undef -> undef
            if (lhs.is_unknown() || rhs.is_unknown()) break;
            // otherwise, too complicated
            MarkOverdefined(ssa);
            return true;
          }
          default: {
            // conservatively mark it overdefined
            MarkOverdefined(ssa);
            return true;
          }
        }
      }
      else if (IsSSA<UnarySSA>(i)) {
        // -undef, !undef, ~undef -> undef
        continue;
      }
      else if (auto ssa = SSADynCast<SelectSSA>(i.get())) {
        const auto &cond = GetValue(ssa->cond());
        auto ans = GetValue(ssa->true_val());
        // undef ? X : Y -> X/Y
        if (cond.is_unknown()) {
          // pick the constant one if there is any
          if (!ans) ans = GetValue(ssa->false_val());
        }
        else if (ans.is_unknown()) {
          if (GetValue(ssa->false_val()).is_unknown()) {
            // cond ? undef : undef -> undef
            continue;
          }
          // otherwise, cond ? undef : X -> X
        }
        else {
          // leave 'ans' as true_val's lattice value
        }
        if (ans) {
          MarkForcedConst(ssa, ans.value());
        }
        else {
          MarkOverdefined(ssa);
        }
        return true;
      }
      else if (IsSSA<LoadSSA>(i)) {
        // A load here means one of two things:
        // 1. a load of undef from a global
        // 2. a load from an unknown pointer.
        // Either way, having it return undef is okay.
        continue;
      }
      else {
        // conservatively mark it overdefined
        MarkOverdefined(i.get());
        return true;
      }
    }
    // Check if we have a branch on an undefined value.
    // If so, we force the branch to go one way or the other
    // to make the successor value live.
    const auto &term = block->insts().back();
    if (auto ssa = SSADynCast<BranchSSA>(term.get())) {
      if (!GetValue(ssa->cond()).is_unknown()) continue;
      // If the input to SCCP is actually branch on undef,
      // fix it to false.
      if (ssa->cond()->IsUndef()) {
        auto mod = MakeModule(ssa->cond()->logger());
        ssa->set_cond(mod.GetInt32(0));
        MarkEdgeExecutable(block,
                           SSACast<BlockSSA>(ssa->false_block().get()));
        return true;
      }
      // Otherwise, it's a branch on a symbolic value which is currently
      // considered to be undef. Make sure some edge is executable, so a
      // branch on 'undef' always flows somewhere.
      auto default_succ = SSACast<BlockSSA>(ssa->false_block().get());
      if (MarkEdgeExecutable(block, default_succ)) return true;
      continue;
    }
  }
  return false;
}

// replace value with constant, returns true if replaced
bool SparseCondConstPropagationPass::TryToReplaceWithConst(Value *val) {
  // get lattice value
  auto it = values_.find(val);
  if (it == values_.end()) return false;
  const auto &lv = it->second;
  if (lv.is_overdefined()) return false;
  // read constant value, or make an undef
  auto mod = MakeModule(val->logger());
  auto const_val = lv ? lv.value() : mod.GetUndef(val->type());
  assert(const_val);
  // replace with constant
  val->ReplaceBy(const_val);
  return true;
}
