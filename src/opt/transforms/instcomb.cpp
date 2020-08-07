#include <algorithm>
#include <vector>
#include <unordered_map>
#include <cstdint>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/helper/cast.h"
#include "opt/helper/inst.h"
#include "opt/helper/const.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

inline void SwapOperands(BinarySSA *bin);

/*
  remove redundant instructions
  reference: LLVM release 1

  TODO: not fully tested!
*/
class InstCombinePass : public FunctionPass {
 public:
  InstCombinePass() {}

  bool RunOnFunction(const FuncPtr &func) override;

  void CleanUp() override {
    assert(worklist_.empty());
    parent_.clear();
    cur_ = nullptr;
    result_ = nullptr;
  }

  void RunOn(AccessSSA &ssa) override;
  void RunOn(BinarySSA &ssa) override;
  void RunOn(CastSSA &ssa) override;
  void RunOn(BranchSSA &ssa) override;
  void RunOn(PhiSSA &ssa) override;
  void RunOn(SelectSSA &ssa) override;

  // This method is to be used when an instruction is found to be dead,
  // replacable with another preexisting expression. Here we add all uses
  // of 'inst' to the worklist, replace all uses of 'inst' with the new
  // value, then return 'inst', so that the inst combiner will known
  // that 'inst' was modified.
  UserPtr ReplaceInstUsesWith(const UserPtr &inst, const SSAPtr &val) {
    AddUsesToWorklist(inst);
    inst->ReplaceBy(val);
    return inst;
  }

 private:
  // erase instruction from it's parent block
  void RemoveFromParent(const UserPtr &inst) {
    parent_[inst.get()]->insts().remove_if(
        [&inst](const SSAPtr &i) { return i == inst; });
  }

  // remove instruction from worklist
  void RemoveFromWorklist(const UserPtr &inst) {
    worklist_.erase(std::remove(worklist_.begin(), worklist_.end(), inst),
                    worklist_.end());
  }

  // add all users of instruction to worklist
  void AddUsesToWorklist(const UserPtr &inst) {
    assert(IsInstruction(inst));
    for (const auto &i : *InstCast(inst.get())) {
      if (!i.user()->uses().empty()) {
        worklist_.push_back(InstCast(i.user()->uses().front()->value()));
      }
    }
  }

  // Insert an instruction New before instruction 'old'
  // in the program. Add the new instruction to the worklist.
  void InsertNewInstBefore(const UserPtr &new_inst, const UserPtr &old) {
    assert(new_inst && !parent_.count(new_inst.get()));
    auto &insts = parent_[old.get()]->insts();
    auto it = std::find(insts.begin(), insts.end(), old);
    insts.insert(it, new_inst);
    parent_[new_inst.get()] = parent_[old.get()];
    worklist_.push_back(new_inst);
  }

  bool SimplifyCommutative(BinarySSA &ssa);
  UserPtr OptAndOp(BinarySSA *opr,
                   const ConstantHelper::ConstIntPtr &op_rhs,
                   const ConstantHelper::ConstIntPtr &and_rhs,
                   const UserPtr &the_and);

  // visitor methods for specific kind of binary instructions
  UserPtr RunOnAdd(BinarySSA &ssa);
  UserPtr RunOnSub(BinarySSA &ssa);
  UserPtr RunOnMul(BinarySSA &ssa);
  UserPtr RunOnDiv(BinarySSA &ssa);
  UserPtr RunOnRem(BinarySSA &ssa);
  UserPtr RunOnAnd(BinarySSA &ssa);
  UserPtr RunOnOr(BinarySSA &ssa);
  UserPtr RunOnXor(BinarySSA &ssa);
  UserPtr RunOnCmp(BinarySSA &ssa);
  UserPtr RunOnShift(BinarySSA &ssa);

  // Perform an optimization on an associative operator. This function is
  // designed to check a chain of associative operators for a potential to
  // apply a certain optimization.  Since the optimization may be applicable
  // if the expression was reassociated, this checks the chain, then
  // reassociates the expression as necessary to expose the optimization
  // opportunity.  This makes use of a special Functor, which must define
  // 'ShouldApply' and 'Apply' methods.
  template <typename Functor>
  UserPtr AssociativeOpt(BinarySSA &root, const Functor &f) {
    auto op = root.op();
    const auto &lhs = root.lhs();
    // Quick check, see if the immediate LHS matches...
    if (f.ShouldApply(lhs)) return f.Apply(root);
    // Otherwise, if the LHS is not of the same opcode as the root, return.
    auto bin = SSADynCast<BinarySSA>(lhs.get());
    while (bin && bin->op() == op && bin->uses().size() == 1) {
      // Should we apply this transform to the RHS?
      bool should_apply = f.ShouldApply(bin->rhs());
      // If not to the RHS, check to see if we should apply to the LHS...
      if (!should_apply && f.ShouldApply(bin->lhs())) {
        // Make the LHS the RHS
        SwapOperands(bin);
        should_apply = true;
      }
      // If the functor wants to apply the optimization to the RHS of bin,
      // reassociate the expression from ((? op A) op B) to (? op (A op B))
      if (should_apply) {
        auto block = parent_[&root];
        auto &insts = block->insts();
        // All of the instructions have a single use and have no
        // side-effects, because of this, we can pull them all into the
        // current basic block.
        if (parent_[bin] != block) {
          // Move all of the instructions from root to LHSI into the
          // current block.
          auto temp_bin = SSACast<BinarySSA>(lhs);
          auto last_use = &root;
          while (parent_[temp_bin.get()] == block) {
            last_use = temp_bin.get();
            temp_bin = SSACast<BinarySSA>(temp_bin->lhs());
          }
          // get the position of 'last_use'
          auto it = std::find_if(
              insts.begin(), insts.end(),
              [last_use](const SSAPtr &v) { return v.get() == last_use; });
          // Loop over all of the instructions in other blocks, moving
          // them into the current one.
          auto temp_bin2 = temp_bin;
          do {
            temp_bin = temp_bin2;
            // Remove from current block...
            parent_[temp_bin.get()]->insts().remove(temp_bin);
            // Insert before the last instruction...
            insts.insert(it, temp_bin);
            parent_[temp_bin.get()] = block;
            temp_bin2 = SSACast<BinarySSA>(temp_bin->lhs());
          } while (temp_bin.get() != bin);
        }
        // Now all of the instructions are in the current basic block,
        // go ahead and perform the reassociation.
        auto temp_bin = SSACast<BinarySSA>(lhs);
        // First move the selected RHS to the LHS of the root...
        root.set_lhs(bin->rhs());
        // Make what used to be the LHS of the root be
        // the user of the root...
        auto extra_operand = temp_bin->rhs();
        assert(!root.uses().empty());
        auto root_ptr = root.uses().front()->value();
        root.ReplaceBy(temp_bin);     // Users now use temp_bin
        temp_bin->set_rhs(root_ptr);  // temp_bin now uses the root
        insts.remove(root_ptr);       // Remove root from the BB
        auto it = std::find(insts.begin(), insts.end(), temp_bin);
        insts.insert(it, root_ptr);   // Insert root before temp_bin
        // Now propagate the 'extra_operand' down the chain of instructions
        // until we get to 'bin'.
        while (temp_bin.get() != bin) {
          auto next_bin = SSACast<BinarySSA>(temp_bin->lhs());
          auto next_op = next_bin->rhs();
          next_bin->set_rhs(extra_operand);
          temp_bin = next_bin;
          extra_operand = next_op;
        }
        // Now that the instructions are reassociated, have the functor
        // perform the transformation...
        return f.Apply(root);
      }
      bin = SSADynCast<BinarySSA>(bin->lhs().get());
    }
    return nullptr;
  }

  std::vector<UserPtr> worklist_;
  std::unordered_map<Value *, BlockSSA *> parent_;
  UserPtr cur_, result_;
};


// get a complexity or rank of Values...
//   0 -> Constant, 1 -> Other, 2 -> Argument, 2 -> Unary, 3 -> OtherInst
inline unsigned GetComplexity(const SSAPtr &val) {
  if (IsSSA<UnarySSA>(val) || IsSSA<UnarySSA>(val)) return 2;
  if (IsInstruction(val)) return 3;
  if (IsSSA<ArgRefSSA>(val)) return 2;
  return val->IsConst() ? 0 : 1;
}

// check if operantion is associative
inline bool IsAssociative(BinarySSA::Operator op) {
  using Op = BinarySSA::Operator;
  switch (op) {
    case Op::Add: case Op::Mul: case Op::And: case Op::Or:
    case Op::Xor: return true;
    default: return false;
  }
}

// check if value is a negate instruction, return it's operand if possible
inline SSAPtr DynCastNeg(const SSAPtr &val) {
  if (auto una = SSADynCast<UnarySSA>(val.get())) {
    if (una->op() == UnarySSA::Operator::Neg) return una->opr();
  }
  // constants can be considered to be negated values if they can be folded
  if (auto cint = ConstantHelper::Fold(val)) {
    return MakeModule(val->logger()).GetInt(-cint->value(), val->type());
  }
  return nullptr;
}

// check for not instruction
inline SSAPtr DynCastNot(const SSAPtr &val) {
  if (auto una = SSADynCast<UnarySSA>(val.get())) {
    if (una->op() == UnarySSA::Operator::Not) return una->opr();
  }
  // constants can be considered to be not'ed values...
  if (auto cint = ConstantHelper::Fold(val)) {
    return MakeModule(val->logger()).GetInt(~cint->value(), val->type());
  }
  return nullptr;
}

// If this value is a multiply that can be folded into other computations
// (because it has a constant operand), return the non-constant operand.
inline SSAPtr DynCastFoldableMul(const SSAPtr &val) {
  if (val->uses().size() == 1 && val->type()->IsInteger()) {
    if (auto bin = SSADynCast<BinarySSA>(val.get())) {
      if (bin->op() == BinarySSA::Operator::Mul) {
        if (bin->rhs()->IsConst()) return bin->lhs();
      }
    }
  }
  return nullptr;
}

// If this value is an 'and' instruction masking a value with a constant,
// return the constant being anded with.
inline ConstantHelper::ConstIntPtr DynCastMaskingAnd(const SSAPtr &val) {
  if (auto bin = SSADynCast<BinarySSA>(val.get())) {
    if (bin->op() == BinarySSA::Operator::And) {
      return ConstantHelper::Fold(bin->rhs());
    }
  }
  // if this is a constant, it acts just like we were masking with it
  return ConstantHelper::Fold(val);
}

// Calculate the log base 2 for the specified value if it is exactly a
// power of 2.
inline unsigned Log2(std::uint32_t val) {
  assert(val > 1 && "Values 0 and 1 should be handled elsewhere!");
  unsigned count = 0;
  while (val != 1) {
    // multiple bits set?
    if (val & 1) return 0;
    val >>= 1;
    ++count;
  }
  return count;
}

// Return true if this instruction will be deleted if we stop using it.
inline bool IsOnlyUse(const SSAPtr &val) {
  return val->uses().size() == 1 || val->IsConst();
}
inline bool IsOnlyUse(Value *val) {
  return val->uses().size() == 1 || val->IsConst();
}

// get inversed comparison operator
inline BinarySSA::Operator InverseCmpOp(BinarySSA::Operator op) {
  using Op = BinarySSA::Operator;
  switch (op) {
    case Op::Equal: return Op::NotEq;
    case Op::NotEq: return Op::Equal;
    case Op::ULess: return Op::UGreatEq;
    case Op::SLess: return Op::SGreatEq;
    case Op::ULessEq: return Op::UGreat;
    case Op::SLessEq: return Op::SGreat;
    case Op::UGreat: return Op::ULessEq;
    case Op::SGreat: return Op::SLessEq;
    case Op::UGreatEq: return Op::ULess;
    case Op::SGreatEq: return Op::SLess;
    default: assert(false); return Op::Add;
  }
}

// Return true if the specified comparison operator is
// true when both operands are equal...
inline bool IsTrueWhenEqual(BinarySSA::Operator op) {
  using Op = BinarySSA::Operator;
  switch (op) {
    case Op::Equal: case Op::ULessEq: case Op::SLessEq: case Op::UGreatEq:
    case Op::SGreatEq: return true;;
    default: return false;
  }
}

// Encode a comparison opcode into a three bit mask. These bits
// are carefully arranged to allow folding of expressions such as:
//
//      (A < B) | (A > B) -> (A != B)
//
// Bit value '4' represents that the comparison is true if A > B,
// bit value '2' represents that the comparison is true if A == B,
// and bit value '1' is true if A < B.
inline unsigned GetCmpCode(BinarySSA::Operator op) {
  using Op = BinarySSA::Operator;
  switch (op) {
    // false -> 0
    case Op::UGreat: case Op::SGreat:     return 1;
    case Op::Equal:                       return 2;
    case Op::UGreatEq: case Op::SGreatEq: return 3;
    case Op::ULess: case Op::SLess:       return 4;
    case Op::NotEq:                       return 5;
    case Op::ULessEq: case Op::SLessEq:   return 6;
    // true -> 7
    default: assert(false); return 0;
  }
}

// This is the complement of 'GetCmpCode', which turns an opcode and
// two operands into either a constant true or false, or a brand new
// comparison instruction.
inline SSAPtr GetCmpValue(unsigned code, const SSAPtr &lhs,
                          const SSAPtr &rhs) {
  auto mod = MakeModule(lhs->logger());
  switch (code) {
    case 0: return mod.GetInt32(0);
    case 1: return mod.CreateGreat(lhs, rhs);
    case 2: return mod.CreateEqual(lhs, rhs);
    case 3: return mod.CreateGreatEq(lhs, rhs);
    case 4: return mod.CreateLess(lhs, rhs);
    case 5: return mod.CreateNotEq(lhs, rhs);
    case 6: return mod.CreateLessEq(lhs, rhs);
    case 7: return mod.GetInt32(1);
    default: assert(false); return nullptr;
  }
}

// swap two operands of binary instruction
// also swap operator
inline void SwapOperands(BinarySSA *bin) {
  // swap operands
  auto temp = bin->lhs();
  bin->set_lhs(bin->rhs());
  bin->set_rhs(temp);
  // handle operator
  switch (bin->op()) {
    case BinarySSA::Operator::Add: case BinarySSA::Operator::Mul:
    case BinarySSA::Operator::And: case BinarySSA::Operator::Or:
    case BinarySSA::Operator::Xor: {
      // does not change
      break;
    }
    default: {
      if (bin->IsCmp()) {
        bin->set_op(InverseCmpOp(bin->op()));
      }
      else {
        assert(false && "unsupported operator");
      }
      break;
    }
  }
}

// Return true if the value represented by the constant only has the
// highest order bit set.
inline bool IsSignBit(const ConstantHelper::ConstIntPtr &c) {
  auto num_bits = c->type()->GetSize() * 8;
  return (c->value() & ~(-1 << num_bits)) ==
         (static_cast<std::uint32_t>(1) << (num_bits - 1));
}

// check if integer value is min value
inline bool IsMinValue(const ConstantHelper::ConstIntPtr &c) {
  if (c->type()->IsUnsigned()) return !c->value();
  if (c->type()->GetSize() == 1) {
    return static_cast<std::int8_t>(c->value()) == -128;
  }
  else {
    assert(c->type()->GetSize() == 4);
    return c->value() == 0x80000000;
  }
}

// check if integer value is min value plus one
inline bool IsMinValuePlusOne(const ConstantHelper::ConstIntPtr &c) {
  if (c->type()->IsUnsigned()) return c->value() == 1;
  if (c->type()->GetSize() == 1) {
    return static_cast<std::int8_t>(c->value()) == -127;
  }
  else {
    assert(c->type()->GetSize() == 4);
    return c->value() == 0x80000001;
  }
}

// check if integer value is max value
inline bool IsMaxValue(const ConstantHelper::ConstIntPtr &c) {
  if (c->type()->GetSize() == 1) {
    return c->type()->IsUnsigned()
               ? static_cast<std::uint8_t>(c->value()) == 255
               : static_cast<std::int8_t>(c->value()) == 127;
  }
  else {
    assert(c->type()->GetSize() == 4);
    return c->type()->IsUnsigned() ? c->value() == 0xffffffff
                                   : c->value() == 0x7fffffff;
  }
}

// check if integer value is max value minus one
inline bool IsMaxValueMinusOne(const ConstantHelper::ConstIntPtr &c) {
  if (c->type()->GetSize() == 1) {
    return c->type()->IsUnsigned()
               ? static_cast<std::uint8_t>(c->value()) == 254
               : static_cast<std::int8_t>(c->value()) == 126;
  }
  else {
    assert(c->type()->GetSize() == 4);
    return c->type()->IsUnsigned() ? c->value() == 0xfffffffe
                                   : c->value() == 0x7ffffffe;
  }
}

// Add or subtract a constant one from an integer constant...
inline SSAPtr AddOne(const ConstantHelper::ConstIntPtr &c) {
  return MakeModule(c->logger()).GetInt(c->value() + 1, c->type());
}
inline SSAPtr SubOne(const ConstantHelper::ConstIntPtr &c) {
  return MakeModule(c->logger()).GetInt(c->value() - 1, c->type());
}

// implements X + X -> X << 1
struct AddRhs {
  AddRhs(const SSAPtr &rhs) : rhs(rhs) {}
  bool ShouldApply(const SSAPtr &lhs) const { return lhs == rhs; }
  UserPtr Apply(BinarySSA &add) const {
    auto mod = MakeModule(add.logger());
    return mod.CreateShl(add.lhs(), mod.GetInt(1, add.type()));
  }

  SSAPtr rhs;
};

// implements (A & C1) + (B & C2) -> (A & C1) | (B & C2)
// iff C1 & C2 == 0
struct AddMaskingAnd {
  AddMaskingAnd(const ConstantHelper::ConstIntPtr c) : c2(c) {}
  bool ShouldApply(const SSAPtr &lhs) const {
    if (auto c1 = DynCastMaskingAnd(lhs)) {
      return !(c1->value() & c2->value());
    }
    return false;
  }
  UserPtr Apply(BinarySSA &add) const {
    auto mod = MakeModule(add.logger());
    return mod.CreateOr(add.lhs(), add.rhs());
  }

  ConstantHelper::ConstIntPtr c2;
};

// implements (A cmp1 B) & (A cmp2 B) -> (A cmp3 B)
struct FoldCmpLogical {
  FoldCmpLogical(InstCombinePass &ic, BinarySSA *cmp)
      : ic(ic), lhs(cmp->lhs()), rhs(cmp->rhs()) {}
  bool ShouldApply(const SSAPtr &lhs) const {
    if (auto bin = SSADynCast<BinarySSA>(lhs.get())) {
      if (!bin->IsCmp()) return false;
      return (bin->lhs() == lhs && bin->rhs() == rhs) ||
             (bin->lhs() == rhs && bin->rhs() == lhs);
    }
    return false;
  }
  UserPtr Apply(BinarySSA &log) const {
    auto bin = SSACast<BinarySSA>(log.lhs().get());
    if (bin->lhs() != lhs) {
      assert(bin->rhs() == lhs);
      SwapOperands(bin);
    }
    auto lhs_code = GetCmpCode(bin->op());
    auto rhs_code = GetCmpCode(SSACast<BinarySSA>(bin->rhs().get())->op());
    unsigned code;
    switch (log.op()) {
      case BinarySSA::Operator::And:  code = lhs_code & rhs_code; break;
      case BinarySSA::Operator::Or:   code = lhs_code | rhs_code; break;
      case BinarySSA::Operator::Xor:  code = lhs_code ^ rhs_code; break;
      default: assert(false); return nullptr;
    }
    auto rv = GetCmpValue(code, lhs, rhs);
    if (!rv->IsConst()) return InstCast(rv);
    // otherwise, it's a constant boolean value (int32 0/1)
    assert(!log.uses().empty());
    auto log_inst = InstCast(log.uses().front()->value());
    return ic.ReplaceInstUsesWith(log_inst, rv);
  }

  InstCombinePass &ic;
  SSAPtr lhs, rhs;
};

}  // namespace

// register current pass
REGISTER_PASS(InstCombinePass, inst_comb)
    .set_min_opt_level(1)
    .set_stages(PassStage::Opt);


// This performs a few simplifications for commutative
// operators:
//
//  1. Order operands such that they are listed from right (least complex)
//     to left (most complex). This puts constants before unary operators
//     before binary operators.
//  2. Transform: ((V op C1) op C2) ==> (V op (C1 op C2))
//  3. Transform: ((V1 op C1) op (V2 op C2))
//            ==> ((V1 op V2) op (C1 op C2))
bool InstCombinePass::SimplifyCommutative(BinarySSA &ssa) {
  bool changed = false;
  if (GetComplexity(ssa.lhs()) < GetComplexity(ssa.rhs())) {
    SwapOperands(&ssa);
    changed = true;
  }
  if (!IsAssociative(ssa.op())) return changed;
  auto op = ssa.op();
  if (auto opr = SSADynCast<BinarySSA>(ssa.lhs().get())) {
    if (opr->op() == op && opr->rhs()->IsConst()) {
      if (ssa.rhs()->IsConst()) {
        auto folded = ConstantHelper::Fold(op, opr->rhs(), ssa.rhs());
        ssa.set_lhs(opr->lhs());
        ssa.set_rhs(folded);
        return true;
      }
      else if (auto opr1 = SSADynCast<BinarySSA>(ssa.rhs().get())) {
        if (opr1->op() == op && opr1->rhs()->IsConst() && IsOnlyUse(opr) &&
            IsOnlyUse(opr1)) {
          auto c1 = ConstantHelper::Fold(opr->rhs());
          auto c2 = ConstantHelper::Fold(opr1->rhs());
          // fold ((V1 op C1) op (V2 op C2))
          //  ==> ((V1 op V2) op (C1 op C2))
          auto folded = ConstantHelper::Fold(op, c1, c2);
          auto mod = MakeModule(ssa.logger());
          auto inst = mod.CreateBinary(op, opr->lhs(), opr1->lhs(),
                                       ssa.type());
          worklist_.push_back(inst);
          ssa.set_lhs(inst);
          ssa.set_rhs(folded);
          return true;
        }
      }
    }
  }
  return changed;
}

// This handles expressions of the form ((val OP C1) & C2). Where
// the opr parameter is 'OP', op_rhs is 'C1', and and_rhs is 'C2'. Opr is
// guaranteed to be either a shift instruction or a binary operator.
UserPtr InstCombinePass::OptAndOp(
    BinarySSA *opr, const ConstantHelper::ConstIntPtr &op_rhs,
    const ConstantHelper::ConstIntPtr &and_rhs, const UserPtr &the_and) {
  const auto &x = opr->lhs();
  auto mod = MakeModule(opr->logger());
  switch (opr->op()) {
    case BinarySSA::Operator::Xor: {
      // (X ^ C1) & C2 -> (X & C2) iff (C1 & C2) == 0
      if (!(and_rhs->value() & op_rhs->value())) {
        return mod.CreateAnd(x, and_rhs);
      }
      // (X ^ C1) & C2 -> (X & C2) ^ (C1 & C2)
      if (opr->uses().size() == 1) {
        auto inst = mod.CreateAnd(x, and_rhs);
        InsertNewInstBefore(inst, the_and);
        auto v = mod.GetInt(and_rhs->value() & op_rhs->value(),
                            opr->type());
        return mod.CreateXor(inst, v);
      }
      break;
    }
    case BinarySSA::Operator::Or: {
      // (X | C1) & C2 --> X & C2 iff C1 & C1 == 0
      if (!(and_rhs->value() & op_rhs->value())) {
        return mod.CreateAnd(x, and_rhs);
      }
      else {
        auto together = mod.GetInt(and_rhs->value() & op_rhs->value(),
                                   opr->type());
        if (ConstantHelper::IsIdentical(together, and_rhs)) {
          return ReplaceInstUsesWith(the_and, and_rhs);
        }
        if (opr->uses().size() == 1 &&
            !ConstantHelper::IsIdentical(together, op_rhs)) {
          // (X | C1) & C2 --> (X | (C1 & C2)) & C2
          auto inst = mod.CreateOr(x, together);
          InsertNewInstBefore(inst, the_and);
          return mod.CreateAnd(inst, and_rhs);
        }
      }
      break;
    }
    case BinarySSA::Operator::Add: {
      if (opr->uses().size() == 1) {
        // Adding a one to a single bit bit-field should be turned into
        // an XOR of the bit. First thing to check is to see if this AND
        // is with a single bit constant.
        auto and_rhsv = and_rhs->value();
        // Clear bits that are not part of the constant.
        and_rhsv &= (1 << and_rhs->type()->GetSize() * 8) - 1;
        // If there is only one bit set...
        if (!(and_rhsv & (and_rhsv - 1))) {
          // Ok, at this point, we know that we are masking the result of
          // the ADD down to exactly one bit. If the constant we are adding
          // has no bits set below this bit, then we can eliminate the ADD.
          auto add_rhs = op_rhs->value();
          // Check to see if any bits below the one bit set in AndRHSV
          // are set.
          if (!(add_rhs & (and_rhsv - 1))) {
            // If not, the only thing that can effect the output of the AND
            // is the bit specified by AndRHSV. If that bit is set, the
            // effect of the XOR is to toggle the bit. If it is clear, then
            // the ADD has no effect.
            if (!(add_rhs & and_rhsv)) { // Bit is not set, noop
              (*the_and)[0].set_value(x);
              return the_and;
            }
            else {
              // Pull the XOR out of the AND.
              auto inst = mod.CreateAnd(x, and_rhs);
              InsertNewInstBefore(inst, the_and);
              return mod.CreateXor(inst, and_rhs);
            }
          }
        }
      }
      break;
    }
    case BinarySSA::Operator::Shl: {
      // We know that the AND will not produce any of the bits shifted in,
      // so if the anded constant includes them, clear them now!
      auto c = and_rhs->value() &
               (static_cast<std::uint32_t>(-1) << op_rhs->value());
      if (c != and_rhs->value()) {
        (*the_and)[1].set_value(mod.GetInt(c, and_rhs->type()));
        return the_and;
      }
      break;
    }
    case BinarySSA::Operator::LShr: {
      // We know that the AND will not produce any of the bits shifted in,
      // so if the anded constant includes them, clear them now! This only
      // applies to unsigned shifts, because a signed shr may bring in
      // set bits!
      auto all_one = (1 << and_rhs->type()->GetSize() * 8) - 1;
      auto c = and_rhs->value() & (all_one >> op_rhs->value());
      if (c != and_rhs->value()) {
        (*the_and)[1].set_value(mod.GetInt(c, and_rhs->type()));
        return the_and;
      }
      break;
    }
    default:;
  }
  return nullptr;
}

UserPtr InstCombinePass::RunOnAdd(BinarySSA &ssa) {
  bool changed = SimplifyCommutative(ssa);
  const auto &lhs = ssa.lhs(), &rhs = ssa.rhs();
  auto mod = MakeModule(ssa.logger());
  // X + 0 -> X
  if (ConstantHelper::IsZero(rhs)) return ReplaceInstUsesWith(cur_, lhs);
  // X + X -> X << 1
  if (ssa.type()->IsInteger()) {
    if (auto result = AssociativeOpt(ssa, AddRhs(rhs))) return result;
  }
  // -A + B -> B - A
  if (auto v = DynCastNeg(lhs)) return mod.CreateSub(rhs, v);
  // A + -B -> A - B
  if (!rhs->IsConst()) {
    if (auto v = DynCastNeg(rhs)) return mod.CreateSub(lhs, v);
  }
  // X * C + X -> X * (C + 1)
  if (DynCastFoldableMul(lhs) == rhs) {
    auto c = ConstantHelper::Fold(SSACast<BinarySSA>(lhs.get())->rhs());
    auto cp1 = mod.GetInt(c->value() + 1, ssa.type());
    return mod.CreateMul(rhs, cp1);
  }
  // X + X * C -> X * (C + 1)
  if (DynCastFoldableMul(rhs) == lhs) {
    auto c = ConstantHelper::Fold(SSACast<BinarySSA>(rhs.get())->rhs());
    auto cp1 = mod.GetInt(c->value() + 1, ssa.type());
    return mod.CreateMul(lhs, cp1);
  }
  // (A & C1) + (B & C2) -> (A & C1) | (B & C2) iff C1 & C2 == 0
  if (auto c2 = DynCastMaskingAnd(rhs)) {
    if (auto result = AssociativeOpt(ssa, AddMaskingAnd(c2))) return result;
  }
  // ~X + C -> (C - 1) - X
  if (rhs->IsConst() && !lhs->IsConst()) {
    auto cint = ConstantHelper::Fold(rhs);
    if (auto v = DynCastNot(lhs)) {
      return mod.CreateSub(mod.GetInt(cint->value() - 1, ssa.type()), v);
    }
  }
  return changed ? cur_ : nullptr;
}

UserPtr InstCombinePass::RunOnSub(BinarySSA &ssa) {
  const auto &lhs = ssa.lhs(), &rhs = ssa.rhs();
  auto mod = MakeModule(ssa.logger());
  // X - X -> 0
  if (lhs == rhs) {
    return ReplaceInstUsesWith(cur_, mod.GetInt(0, ssa.type()));
  }
  // X - (-A) -> X + A
  if (auto v = DynCastNeg(rhs)) return mod.CreateAdd(lhs, v);
  // -1 - A -> ~A
  if (auto cint = ConstantHelper::Fold(lhs)) {
    if (cint->value() == 0xffffffff) return mod.CreateNot(rhs);
  }
  if (auto bin = SSADynCast<BinarySSA>(rhs.get())) {
    if (bin->uses().size() == 1) {
      // X - (Y - Z) -> X + (Z - Y)
      if (bin->op() == BinarySSA::Operator::Sub) {
        // swap the two operands of rhs
        auto temp = bin->lhs();
        bin->set_lhs(bin->rhs());
        bin->set_rhs(temp);
        // make a new add instruction
        return mod.CreateAdd(lhs, rhs);
      }
      // A - (A & B) -> A & ~B
      if (bin->op() == BinarySSA::Operator::And &&
          (bin->lhs() == lhs || bin->rhs() == lhs)) {
        auto other_op = bin->lhs() == lhs ? bin->rhs() : bin->lhs();
        auto new_not = mod.CreateNot(other_op);
        InsertNewInstBefore(new_not, cur_);
        return mod.CreateAnd(lhs, new_not);
      }
      // X - X * C -> X * (1 - C)
      if (DynCastFoldableMul(rhs) == lhs) {
        auto c = ConstantHelper::Fold(SSACast<BinarySSA>(rhs.get())->rhs());
        auto ncp1 = mod.GetInt(1 - c->value(), ssa.type());
        return mod.CreateMul(lhs, ncp1);
      }
    }
  }
  // X * C - X -> X * (C - 1)
  if (DynCastFoldableMul(lhs) == rhs) {
    auto c = ConstantHelper::Fold(SSACast<BinarySSA>(lhs.get())->rhs());
    auto cm1 = mod.GetInt(c->value() - 1, ssa.type());
    return mod.CreateMul(rhs, cm1);
  }
  return nullptr;
}

UserPtr InstCombinePass::RunOnMul(BinarySSA &ssa) {
  bool changed = SimplifyCommutative(ssa);
  const auto &lhs = ssa.lhs();
  auto mod = MakeModule(ssa.logger());
  // simplify 'mul' instructions with a constant rhs
  if (auto cint = ConstantHelper::Fold(ssa.rhs())) {
    // (X << C1) * C2 -> X * (C2 << C1)
    if (auto bin = SSADynCast<BinarySSA>(lhs.get())) {
      if (bin->op() == BinarySSA::Operator::Shl) {
        if (auto c1 = ConstantHelper::Fold(bin->rhs())) {
          auto c = mod.GetInt(cint->value() << c1->value(), ssa.type());
          return mod.CreateMul(bin->lhs(), c);
        }
      }
    }
    // X * 0 -> 0
    if (!cint->value()) return ReplaceInstUsesWith(cur_, cint);
    // X * 1 -> X
    if (cint->value() == 1) return ReplaceInstUsesWith(cur_, lhs);
    // X * -1 = -X
    if (cint->value() == 0xffffffff) return mod.CreateNeg(lhs);
    // X * (2 ** C) -> X << C
    if (auto c = Log2(cint->value())) {
      return mod.CreateShl(lhs, mod.GetInt(c, ssa.type()));
    }
  }
  // -X * -Y -> X * Y
  if (auto l = DynCastNeg(lhs)) {
    if (auto r = DynCastNeg(ssa.rhs())) return mod.CreateMul(l, r);
  }
  return changed ? cur_ : nullptr;
}

UserPtr InstCombinePass::RunOnDiv(BinarySSA &ssa) {
  auto mod = MakeModule(ssa.logger());
  if (auto cint = ConstantHelper::Fold(ssa.rhs())) {
    // X / 1 -> X
    if (cint->value() == 1) return ReplaceInstUsesWith(cur_, ssa.lhs());
    // X / (2 ** C) -> X >> C, iff is unsigned division
    // NOTE: do not break X / 0
    if (ssa.op() == BinarySSA::Operator::UDiv && cint->value()) {
      if (auto c = Log2(cint->value())) {
        return mod.CreateShr(ssa.lhs(), mod.GetInt(c, ssa.type()));
      }
    }
  }
  // 0 / X -> 0
  if (auto cint = ConstantHelper::Fold(ssa.lhs())) {
    if (!cint->value()) return ReplaceInstUsesWith(cur_, cint);
  }
  return nullptr;
}

UserPtr InstCombinePass::RunOnRem(BinarySSA &ssa) {
  auto mod = MakeModule(ssa.logger());
  if (auto cint = ConstantHelper::Fold(ssa.rhs())) {
    // X % 1 -> 0
    if (cint->value() == 1) {
      return ReplaceInstUsesWith(cur_, mod.GetInt(0, ssa.type()));
    }
    // X % (2 ** C) -> X & (C - 1), iff is unsigned remainder
    // NOTE: do not break X % 0
    if (ssa.op() == BinarySSA::Operator::URem && cint->value()) {
      auto v = cint->value();
      if (Log2(v)) {
        return mod.CreateAnd(ssa.lhs(), mod.GetInt(v - 1, ssa.type()));
      }
    }
  }
  // 0 % X -> 0
  if (auto cint = ConstantHelper::Fold(ssa.lhs())) {
    if (!cint->value()) return ReplaceInstUsesWith(cur_, cint);
  }
  return nullptr;
}

UserPtr InstCombinePass::RunOnAnd(BinarySSA &ssa) {
  bool changed = SimplifyCommutative(ssa);
  const auto &lhs = ssa.lhs(), &rhs = ssa.rhs();
  auto crhs = ConstantHelper::Fold(rhs);
  auto mod = MakeModule(ssa.logger());
  // X & X -> X, X & 0 -> 0
  if (lhs == rhs || (crhs && !crhs->value())) {
    return ReplaceInstUsesWith(cur_, rhs);
  }
  if (crhs) {
    // X & -1 -> X
    if (crhs->value() == 0xffffffff) return ReplaceInstUsesWith(cur_, lhs);
    // optimize a variety of (X op C1) & C2 combinations
    if (auto bin = SSADynCast<BinarySSA>(lhs.get())) {
      if (auto cint = ConstantHelper::Fold(bin->rhs())) {
        if (auto result = OptAndOp(bin, cint, crhs, cur_)) return result;
      }
    }
  }
  auto nlhs = DynCastNot(lhs), nrhs = DynCastNot(rhs);
  // ~A & ~B -> ~(A | B), Demorgan's Law
  if (nlhs && nrhs && IsOnlyUse(lhs) && IsOnlyUse(rhs)) {
    auto or_inst = mod.CreateOr(nlhs, nrhs);
    InsertNewInstBefore(or_inst, cur_);
    return mod.CreateNot(or_inst);
  }
  // A & ~A -> 0, ~A & A -> 0
  if (nlhs == rhs || nrhs == lhs) {
    return ReplaceInstUsesWith(cur_, mod.GetInt(0, ssa.type()));
  }
  // (A cmp1 B) & (A cmp2 B) -> (A cmp3 B)
  if (auto cmp = SSADynCast<BinarySSA>(rhs.get())) {
    if (cmp->IsCmp()) {
      if (auto result = AssociativeOpt(ssa, FoldCmpLogical(*this, cmp))) {
        return result;
      }
    }
  }
  return changed ? cur_ : nullptr;
}

UserPtr InstCombinePass::RunOnOr(BinarySSA &ssa) {
  bool changed = SimplifyCommutative(ssa);
  const auto &lhs = ssa.lhs(), &rhs = ssa.rhs();
  auto crhs = ConstantHelper::Fold(rhs);
  auto mod = MakeModule(ssa.logger());
  // X | X -> X, X | 0 -> X
  if (lhs == rhs || (crhs && !crhs->value())) {
    return ReplaceInstUsesWith(cur_, lhs);
  }
  if (crhs) {
    // X | -1 -> -1
    if (crhs->value() == 0xffffffff) return ReplaceInstUsesWith(cur_, crhs);
    if (auto bin = SSADynCast<BinarySSA>(lhs.get())) {
      // (X & C1) | C2 -> (X | C2) & (C1 | C2)
      if (bin->op() == BinarySSA::Operator::And && IsOnlyUse(lhs)) {
        if (auto c1 = ConstantHelper::Fold(bin->rhs())) {
          auto or_inst = mod.CreateOr(bin->lhs(), rhs);
          InsertNewInstBefore(or_inst, cur_);
          auto c = mod.GetInt(c1->value() | crhs->value(), ssa.type());
          return mod.CreateAnd(or_inst, c);
        }
      }
      // (X ^ C1) | C2 -> (X | C2) ^ (C1 & ~C2)
      if (bin->op() == BinarySSA::Operator::Xor && IsOnlyUse(lhs)) {
        if (auto c1 = ConstantHelper::Fold(bin->rhs())) {
          auto or_inst = mod.CreateOr(bin->lhs(), rhs);
          InsertNewInstBefore(or_inst, cur_);
          auto c = mod.GetInt(c1->value() & ~crhs->value(), ssa.type());
          return mod.CreateAnd(or_inst, c);
        }
      }
    }
  }
  // (A & C1) | (A & C2) -> A & (C1 | C2)
  if (auto lb = SSADynCast<BinarySSA>(lhs.get())) {
    if (auto rb = SSADynCast<BinarySSA>(rhs.get())) {
      if (lb->lhs() == rb->lhs()) {
        if (auto c1 = ConstantHelper::Fold(lb->rhs())) {
          if (auto c2 = ConstantHelper::Fold(rb->rhs())) {
            auto c = mod.GetInt(c1->value() | c2->value(), ssa.type());
            return mod.CreateAnd(lb->lhs(), c);
          }
        }
      }
    }
  }
  auto nlhs = DynCastNot(lhs), nrhs = DynCastNot(rhs);
  // ~A | A -> -1, A | ~A -> -1
  if (rhs == nlhs || lhs == nrhs) {
    return ReplaceInstUsesWith(cur_, mod.GetInt(-1, ssa.type()));
  }
  // (~A | ~B) -> ~(A & B), Demorgan's Law
  if (nlhs && nrhs && IsOnlyUse(lhs) && IsOnlyUse(rhs)) {
    auto and_inst = mod.CreateAnd(nlhs, nrhs);
    worklist_.push_back(and_inst);
    return mod.CreateNot(and_inst);
  }
  // (A cmp1 B) | (A cmp2 B) -> (A cmp3 B)
  if (auto cmp = SSADynCast<BinarySSA>(rhs.get())) {
    if (cmp->IsCmp()) {
      if (auto result = AssociativeOpt(ssa, FoldCmpLogical(*this, cmp))) {
        return result;
      }
    }
  }
  return changed ? cur_ : nullptr;
}

UserPtr InstCombinePass::RunOnXor(BinarySSA &ssa) {
  bool changed = SimplifyCommutative(ssa);
  const auto &lhs = ssa.lhs(), &rhs = ssa.rhs();
  auto mod = MakeModule(ssa.logger());
  // X ^ X -> 0
  if (lhs == rhs) {
    return ReplaceInstUsesWith(cur_, mod.GetInt(0, ssa.type()));
  }
  if (auto crhs = ConstantHelper::Fold(rhs)) {
    // X ^ 0 -> X
    if (!crhs->value()) return ReplaceInstUsesWith(cur_, lhs);
    if (auto bin = SSADynCast<BinarySSA>(lhs.get())) {
      // (A cmp B) ^ true -> !(A cmp B) -> (A !cmp B)
      if (bin->IsCmp()) {
        if (crhs->value() && bin->uses().size() == 1) {
          return mod.CreateBinary(InverseCmpOp(bin->op()), bin->lhs(),
                                  bin->rhs(), ssa.type());
        }
      }
      if (auto c1 = ConstantHelper::Fold(bin->rhs())) {
        // (X & C1) ^ C2 -> (X & C1) | C2 iff (C1 & C2) == 0
        if (bin->op() == BinarySSA::Operator::And) {
          if (!(c1->value() & crhs->value())) return mod.CreateOr(lhs, rhs);
        }
        // (X | C1) ^ C2 -> (X | C1) & ~C2 iff (C1 & C2) == C2
        if (bin->op() == BinarySSA::Operator::Or) {
          if ((c1->value() & crhs->value()) == crhs->value()) {
            auto c = mod.GetInt(~crhs->value(), ssa.type());
            return mod.CreateAnd(lhs, c);
          }
        }
      }
    }
  }
  // ~A ^ A -> -1, A ^ ~A -> -1
  auto nlhs = DynCastNot(lhs), nrhs = DynCastNot(rhs);
  if (nlhs == rhs || nrhs == lhs) {
    return ReplaceInstUsesWith(cur_, mod.GetInt(-1, ssa.type()));
  }
  if (auto bin = SSADynCast<BinarySSA>(rhs.get())) {
    if (bin->op() == BinarySSA::Operator::Or) {
      // B ^ (B | A) -> (A | B) ^ B
      if (bin->lhs() == lhs) {
        SwapOperands(bin);
        SwapOperands(&ssa);
      }
      // B ^ (A | B) -> (A | B) ^ B
      if (bin->rhs() == lhs) SwapOperands(&ssa);
    }
  }
  if (auto bin = SSADynCast<BinarySSA>(lhs.get())) {
    if (bin->op() == BinarySSA::Operator::Or && bin->uses().size() == 1) {
      // (B | A) ^ B -> (A | B) ^ B
      if (bin->lhs() == rhs) SwapOperands(bin);
      // (A | B) ^ B -> A & ~B
      if (bin->rhs() == rhs) {
        auto not_inst = mod.CreateNot(rhs);
        worklist_.push_back(not_inst);
        return mod.CreateAnd(bin->lhs(), not_inst);
      }
    }
  }
  // (A & C1) ^ (B & C2) -> (A & C1) | (B & C2) iff C1 ^ C2 == 0
  if (auto c1 = DynCastMaskingAnd(lhs)) {
    if (auto c2 = DynCastMaskingAnd(rhs)) {
      if (!(c1->value() ^ c2->value())) return mod.CreateOr(lhs, rhs);
    }
  }
  // (A cmp1 B) ^ (A cmp2 B) -> (A cmp3 B)
  if (auto cmp = SSADynCast<BinarySSA>(rhs.get())) {
    if (cmp->IsCmp()) {
      if (auto result = AssociativeOpt(ssa, FoldCmpLogical(*this, cmp))) {
        return result;
      }
    }
  }
  return changed ? cur_ : nullptr;
}

UserPtr InstCombinePass::RunOnCmp(BinarySSA &ssa) {
  bool changed = SimplifyCommutative(ssa);
  const auto &lhs = ssa.lhs(), &rhs = ssa.rhs();
  auto mod = MakeModule(ssa.logger());
  // X cmp X -> true/false
  if (lhs == rhs) {
    auto b = mod.GetInt32(IsTrueWhenEqual(ssa.op()));
    return ReplaceInstUsesWith(cur_, b);
  }
  // <global/alloca> cmp 0 - global/Stack value addresses are never null!
  if (ConstantHelper::IsZero(lhs) &&
      (IsSSA<GlobalVarSSA>(lhs) || IsSSA<AllocaSSA>(lhs))) {
    auto b = mod.GetInt32(!IsTrueWhenEqual(ssa.op()));
    return ReplaceInstUsesWith(cur_, b);
  }
  // Check to see if we are doing one of many comparisons against constant
  // integers at the end of their ranges...
  if (auto cint = ConstantHelper::Fold(rhs)) {
    // Simplify seteq and setne instructions...
    if (ssa.op() == BinarySSA::Operator::Equal ||
        ssa.op() == BinarySSA::Operator::NotEq) {
      bool is_neq = ssa.op() == BinarySSA::Operator::NotEq;
      // If the first operand is (and|or|xor) with a constant,
      // and the second operand is a constant, simplify a bit.
      if (auto bo = SSADynCast<BinarySSA>(lhs.get())) {
        switch (bo->op()) {
          case BinarySSA::Operator::Add: {
            if (!cint->value()) {
              // (A + B) != 0 -> (A != -B) if A or B is efficiently
              // invertible, or if the add has just this one use.
              const auto &blhs = bo->lhs(), &brhs = bo->rhs();
              if (auto neg = DynCastNeg(brhs)) {
                return mod.CreateBinary(ssa.op(), blhs, neg, ssa.type());
              }
              if (auto neg = DynCastNeg(blhs)) {
                return mod.CreateBinary(ssa.op(), neg, brhs, ssa.type());
              }
              if (bo->uses().size() == 1) {
                auto neg = mod.CreateNeg(brhs);
                InsertNewInstBefore(neg, cur_);
                return mod.CreateBinary(ssa.op(), blhs, neg, ssa.type());
              }
            }
            break;
          }
          case BinarySSA::Operator::Xor: {
            // For the xor case, we can xor two constants together,
            // eliminating the explicit xor.
            if (auto c = ConstantHelper::Fold(bo->rhs())) {
              auto v = mod.GetInt(cint->value() ^ c->value(), ssa.type());
              return mod.CreateBinary(ssa.op(), bo->lhs(), v, ssa.type());
            }
            // fall through
          }
          case BinarySSA::Operator::Sub: {
            // Replace ((A [sub|xor] B) != 0) with (A != B)
            if (!cint->value()) {
              return mod.CreateBinary(ssa.op(), bo->lhs(), bo->rhs(),
                                      ssa.type());
            }
            break;
          }
          case BinarySSA::Operator::Or: {
            // If bits are being or'd in that are not present in the
            // constant we are comparing against, then the comparison
            // could never succeed!
            if (auto c = ConstantHelper::Fold(bo->rhs())) {
              if ((c->value() & ~cint->value())) {
                return ReplaceInstUsesWith(cur_, mod.GetInt32(is_neq));
              }
            }
            break;
          }
          case BinarySSA::Operator::And: {
            if (auto c = ConstantHelper::Fold(bo->rhs())) {
              // If bits are being compared against that are and'd out,
              // then the comparison can never succeed!
              if ((cint->value() & ~c->value())) {
                return ReplaceInstUsesWith(cur_, mod.GetInt32(is_neq));
              }
              // Replace (X and (1 << size(X)-1) != 0) with x < 0,
              // converting X to be a signed value as appropriate.
              if (IsSignBit(c) && !bo->lhs()->IsConst()) {
                auto x = bo->lhs();
                // If 'X' is not signed, insert a cast now...
                if (x->type()->IsUnsigned()) {
                  using namespace mimic::define;
                  PrimType::Type type;
                  switch (x->type()->GetSize()) {
                    case 1: type = PrimType::Type::Int8;
                    case 4: type = PrimType::Type::Int32;
                    default: assert(false);
                  }
                  auto ty = MakePrimType(type, false);
                  auto cast = InstCast(mod.CreateCast(x, ty));
                  InsertNewInstBefore(cast, cur_);
                  x = cast;
                }
                auto zero = mod.GetInt(0, x->type());
                return is_neq ? mod.CreateLess(x, zero)
                              : mod.CreateGreatEq(x, zero);
              }
            }
            break;
          }
          default:;
        }
      }
    }
    // Check to see if we are comparing against the
    // minimum or maximum value...
    if (IsMinValue(cint)) {
      switch (ssa.op()) {
        // A < MIN -> false
        case BinarySSA::Operator::ULess: case BinarySSA::Operator::SLess: {
          return ReplaceInstUsesWith(cur_, mod.GetInt32(0));
        }
        // A >= MIN -> true
        case BinarySSA::Operator::UGreatEq:
        case BinarySSA::Operator::SGreatEq: {
          return ReplaceInstUsesWith(cur_, mod.GetInt32(1));
        }
        // A <= MIN -> A == MIN
        case BinarySSA::Operator::ULessEq:
        case BinarySSA::Operator::SLessEq: {
          return mod.CreateEqual(lhs, rhs);
        }
        // A > MIN -> A != MIN
        case BinarySSA::Operator::UGreat:
        case BinarySSA::Operator::SGreat: {
          return mod.CreateNotEq(lhs, rhs);
        }
        default:;
      }
    }
    else if (IsMaxValue(cint)) {
      switch (ssa.op()) {
        // A > MAX -> false
        case BinarySSA::Operator::UGreat:
        case BinarySSA::Operator::SGreat: {
          return ReplaceInstUsesWith(cur_, mod.GetInt32(0));
        }
        // A <= MAX -> true
        case BinarySSA::Operator::ULessEq:
        case BinarySSA::Operator::SLessEq: {
          return ReplaceInstUsesWith(cur_, mod.GetInt32(1));
        }
        // A >= MAX -> A == MAX
        case BinarySSA::Operator::UGreatEq:
        case BinarySSA::Operator::SGreatEq: {
          return mod.CreateEqual(lhs, rhs);
        }
        // A < MAX -> A != MAX
        case BinarySSA::Operator::ULess: case BinarySSA::Operator::SLess: {
          return mod.CreateNotEq(lhs, rhs);
        }
        default:;
      }
    }
    // Comparing against a value really close to min or max?
    else if (IsMinValuePlusOne(cint)) {
      switch (ssa.op()) {
        // A < MIN + 1 -> A == MIN
        case BinarySSA::Operator::ULess: case BinarySSA::Operator::SLess: {
          return mod.CreateEqual(lhs, SubOne(cint));
        }
        // A >= MIN + 1 -> A != MIN
        case BinarySSA::Operator::UGreatEq:
        case BinarySSA::Operator::SGreatEq: {
          return mod.CreateNotEq(lhs, SubOne(cint));
        }
        default:;
      }
    }
    else if (IsMaxValueMinusOne(cint)) {
      switch (ssa.op()) {
        // A > MAX - 1 -> A == MAX
        case BinarySSA::Operator::UGreat:
        case BinarySSA::Operator::SGreat: {
          return mod.CreateEqual(lhs, AddOne(cint));
        }
        // A <= MAX - 1 -> A != MAX
        case BinarySSA::Operator::ULessEq:
        case BinarySSA::Operator::SLessEq: {
          return mod.CreateNotEq(lhs, AddOne(cint));
        }
        default:;
      }
    }
  }
  return changed ? cur_ : nullptr;
}

UserPtr InstCombinePass::RunOnShift(BinarySSA &ssa) {
  const auto &lhs = ssa.lhs(), &rhs = ssa.rhs();
  bool is_shl = ssa.op() == BinarySSA::Operator::Shl;
  auto mod = MakeModule(ssa.logger());
  // X << 0 -> X, X >> 0 -> X, 0 << X -> 0, 0 >> X -> 0
  if (ConstantHelper::IsZero(lhs) || ConstantHelper::IsZero(rhs)) {
    return ReplaceInstUsesWith(cur_, lhs);
  }
  // -1 >> X -> -1
  if (ssa.op() == BinarySSA::Operator::AShr) {
    if (auto cint = ConstantHelper::Fold(lhs)) {
      if (cint->value() == 0xffffffff) {
        return ReplaceInstUsesWith(cur_, cint);
      }
    }
  }
  if (auto cint = ConstantHelper::Fold(rhs)) {
    // U32 << 32 -> 0, U8 >> 9 -> 0, ... don't eliminate AShr
    auto bits = lhs->type()->GetSize() * 8;
    if (cint->value() >= bits && (lhs->type()->IsUnsigned() || is_shl)) {
      return ReplaceInstUsesWith(cur_, mod.GetInt(0, lhs->type()));
    }
    // (X * C1) << C2 -> (X * (C1 << C2))
    if (auto bo = SSADynCast<BinarySSA>(lhs.get())) {
      if (bo->op() == BinarySSA::Operator::Mul && is_shl) {
        if (auto c1 = ConstantHelper::Fold(bo->rhs())) {
          auto c = mod.GetInt(c1->value() << cint->value(), lhs->type());
          return mod.CreateMul(bo->lhs(), c);
        }
      }
    }
    // If the operand is an bitwise operator with a constant RHS, and the
    // shift is the only use, we can pull it out of the shift.
    if (lhs->uses().size() == 1) {
      if (auto blhs = SSADynCast<BinarySSA>(lhs.get())) {
        if (auto c = ConstantHelper::Fold(blhs->rhs())) {
          // Valid only for And, Or, Xor
          bool is_valid = true;
          // Transform if high bit of constant set?
          bool high_bit_set = false;
          switch (blhs->op()) {
            case BinarySSA::Operator::Or: case BinarySSA::Operator::Xor: {
              high_bit_set = false;
              break;
            }
            case BinarySSA::Operator::And: high_bit_set = true; break;
            default: is_valid = false; break;
          }
          // If this is a signed shift right, and the high bit is modified
          // by the logical operation, do not perform the transformation.
          // The highBitSet boolean indicates the value of the high bit of
          // the constant which would cause it to be modified for this
          // operation.
          if (is_valid && !is_shl && !lhs->type()->IsUnsigned()) {
            auto v = c->value();
            is_valid = ((v & (1 << (bits - 1))) != 0) == high_bit_set;
          }
          if (is_valid) {
            auto new_rhs = ConstantHelper::Fold(ssa.op(), c, cint);
            auto new_shift = mod.CreateBinary(ssa.op(), blhs->lhs(),
                                              cint, blhs->type());
            InsertNewInstBefore(new_shift, cur_);
            return mod.CreateBinary(blhs->op(), new_shift, new_rhs,
                                    lhs->type());
          }
        }
      }
    }
    // If this is a shift of a shift, try to fold the two together...
    if (auto bin = SSADynCast<BinarySSA>(lhs.get())) {
      if (auto c1 = ConstantHelper::Fold(bin->rhs())) {
        auto amt1 = c1->value(), amt2 = cint->value();
        // Check for (A << c1) << c2 and (A >> c1) >> c2
        if (ssa.op() == bin->op()) {
          auto amt = mod.GetInt(amt1 + amt2, ssa.type());
          return mod.CreateBinary(ssa.op(), bin->lhs(), amt, ssa.type());
        }
        // Check for (A << c1) >> c2 or visaversa.  If we are dealing with
        // signed types, we can only support the (A >> c1) << c2
        // configuration, because it can not turn an arbitrary bit of
        // A into a sign bit.
        if (lhs->type()->IsUnsigned() || is_shl) {
          // Calculate bitmask for what gets shifted off the edge...
          auto c = mod.GetInt(-1, lhs->type());
          c = is_shl ? mod.CreateShl(c, c1) : mod.CreateShr(c, c1);
          auto mask = mod.CreateAnd(bin->lhs(), c);
          InsertNewInstBefore(mask, cur_);
          // Figure out what flavor of shift we should use...
          if (amt1 == amt2) {
            // (A << c) >> c  === A & c2
            return ReplaceInstUsesWith(cur_, mask);
          }
          else {
            auto val = mod.GetInt(amt1 < amt2 ? amt2 - amt1 : amt2 - amt1,
                                  rhs->type());
            return mod.CreateBinary(ssa.op(), mask, val, lhs->type());
          }
        }
      }
    }
  }
  return nullptr;
}

void InstCombinePass::RunOn(AccessSSA &ssa) {
  // TODO
}

void InstCombinePass::RunOn(BinarySSA &ssa) {
  using Op = BinarySSA::Operator;
  switch (ssa.op()) {
    case Op::Add: result_ = RunOnAdd(ssa); return;
    case Op::Sub: result_ = RunOnSub(ssa); return;
    case Op::Mul: result_ = RunOnMul(ssa); return;
    case Op::UDiv: result_ = RunOnDiv(ssa); return;
    case Op::SDiv: result_ = RunOnDiv(ssa); return;
    case Op::URem: result_ = RunOnRem(ssa); return;
    case Op::SRem: result_ = RunOnRem(ssa); return;
    case Op::Equal: result_ = RunOnCmp(ssa); return;
    case Op::NotEq: result_ = RunOnCmp(ssa); return;
    case Op::ULess: result_ = RunOnCmp(ssa); return;
    case Op::SLess: result_ = RunOnCmp(ssa); return;
    case Op::ULessEq: result_ = RunOnCmp(ssa); return;
    case Op::SLessEq: result_ = RunOnCmp(ssa); return;
    case Op::UGreat: result_ = RunOnCmp(ssa); return;
    case Op::SGreat: result_ = RunOnCmp(ssa); return;
    case Op::UGreatEq: result_ = RunOnCmp(ssa); return;
    case Op::SGreatEq: result_ = RunOnCmp(ssa); return;
    case Op::And: result_ = RunOnAnd(ssa); return;
    case Op::Or: result_ = RunOnOr(ssa); return;
    case Op::Xor: result_ = RunOnXor(ssa); return;
    case Op::Shl: result_ = RunOnShift(ssa); return;
    case Op::LShr: result_ = RunOnShift(ssa); return;
    case Op::AShr: result_ = RunOnShift(ssa); return;
    default: assert(false); return;
  }
}

void InstCombinePass::RunOn(CastSSA &ssa) {
  // TODO
}

void InstCombinePass::RunOn(BranchSSA &ssa) {
  // branch !X, b1, b2 -> branch X, b2, b1
  if (!ssa.cond()->IsConst()) {
    if (auto v = DynCastNot(ssa.cond())) {
      auto tb = ssa.true_block(), fb = ssa.false_block();
      // Swap Destinations and condition...
      ssa.set_cond(v);
      ssa.set_true_block(fb);
      ssa.set_false_block(tb);
      result_ = cur_;
      return;
    }
  }
  result_ = nullptr;
}

void InstCombinePass::RunOn(PhiSSA &ssa) {
  // If the phi node only has one incoming value, eliminate the phi node...
  if (ssa.size() == 1) {
    auto opr = SSACast<PhiOperandSSA>(ssa[0].value().get())->value();
    result_ = ReplaceInstUsesWith(cur_, opr);
  }
  // Otherwise if all of the incoming values are the same for the phi,
  // replace the phi node with the incoming value.
  SSAPtr val;
  for (const auto &i : ssa) {
    const auto &opr = SSACast<PhiOperandSSA>(i.value().get())->value();
    // Not the phi node itself...
    if (opr.get() != &ssa) {
      if (val && opr != val) {
        // Not the same, bail out
        result_ = nullptr;
        return;
      }
      else {
        val = opr;
      }
    }
  }
  // The only case that could cause InVal to be null is if we have a
  // phi node that only has entries for itself. In this case, there is
  // no entry into the loop, so kill the phi.
  if (!val) val = MakeModule(ssa.logger()).GetUndef(ssa.type());
  // All of the incoming values are the same, replace the PHI node now.
  result_ = ReplaceInstUsesWith(cur_, val);
}

void InstCombinePass::RunOn(SelectSSA &ssa) {
  // select !X, b1, b2 -> select X, b2, b1
  if (!ssa.cond()->IsConst()) {
    if (auto v = DynCastNot(ssa.cond())) {
      auto tv = ssa.true_val(), fv = ssa.false_val();
      // Swap Destinations and condition...
      ssa.set_cond(v);
      ssa.set_true_val(fv);
      ssa.set_false_val(tv);
      result_ = cur_;
      return;
    }
  }
  result_ = nullptr;
}

bool InstCombinePass::RunOnFunction(const FuncPtr &func) {
  bool changed = false;
  // insert all instructions to worklist
  for (const auto &i : *func) {
    auto block = SSACast<BlockSSA>(i.value().get());
    for (const auto &i : block->insts()) {
      if (auto inst = InstDynCast(i)) {
        worklist_.push_back(inst);
        parent_.insert({inst.get(), block});
      }
    }
  }
  // run on worklist
  while (!worklist_.empty()) {
    auto inst = worklist_.back();
    worklist_.pop_back();
    // check to see if we can remove dead instruction
    if (IsInstDead(inst)) {
      // add operands to the worklist
      auto inst_ptr = InstCast(inst.get());
      if (inst_ptr->size() < 4) {
        for (const auto &i : *inst_ptr) {
          if (auto op = InstDynCast(i.value())) {
            worklist_.push_back(op);
          }
        }
      }
      RemoveFromParent(inst);
      RemoveFromWorklist(inst);
      continue;
    }
    // check to see if we can constant propagate it
    if (auto cint = ConstantHelper::Fold(inst)) {
      // add operands to the worklist
      for (const auto &i : *InstCast(inst.get())) {
        if (auto op = InstDynCast(i.value())) {
          worklist_.push_back(op);
        }
      }
      ReplaceInstUsesWith(inst, cint);
      RemoveFromParent(inst);
      RemoveFromWorklist(inst);
      continue;
    }
    // try to combine current instruction
    cur_ = inst;
    result_ = nullptr;
    inst->RunPass(*this);
    if (result_) {
      // check if instruction can be replaced
      if (result_ != inst) {
        // remove all the same instructions from worklist
        RemoveFromWorklist(inst);
        // insert new instruction into block
        auto &insts = parent_[inst.get()]->insts();
        auto it = std::find(insts.begin(), insts.end(), inst);
        *it = result_;
        parent_[result_.get()] = parent_[inst.get()];
        // replace with new instruction
        inst->ReplaceBy(result_);
      }
      else {
        // instruction was modified, check if it's dead
        if (IsInstDead(inst)) {
          RemoveFromParent(inst);
          RemoveFromWorklist(inst);
          result_ = nullptr;
        }
      }
      if (result_) {
        worklist_.push_back(result_);
        AddUsesToWorklist(result_);
      }
      changed = true;
    }
  }
  return changed;
}
