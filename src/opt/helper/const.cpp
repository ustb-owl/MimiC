#include "opt/helper/const.h"

#include <cassert>

#include "mid/module.h"
#include "opt/helper/cast.h"

using namespace mimic::mid;
using namespace mimic::define;
using namespace mimic::opt;

ConstantHelper::ConstIntPtr ConstantHelper::Fold(const SSAPtr &val) {
  if (!val) return nullptr;
  // check if is integer/pointer
  const auto &type = val->type();
  if (!type || (!type->IsInteger() && !type->IsPointer())) return nullptr;
  // handle by SSA type
  if (auto cint = SSADynCast<ConstIntSSA>(val)) {
    return cint;
  }
  else if (auto zero = SSADynCast<ConstZeroSSA>(val)) {
    auto ret = MakeModule(val->logger()).GetInt(0, type);
    return SSACast<ConstIntSSA>(ret);
  }
  else if (auto bin = SSADynCast<BinarySSA>(val)) {
    return Fold(bin->op(), bin->lhs(), bin->rhs());
  }
  else if (auto una = SSADynCast<UnarySSA>(val)) {
    return Fold(una->op(), una->opr());
  }
  else if (auto cast = SSADynCast<CastSSA>(val)) {
    return Fold(cast->opr(), cast->type());
  }
  else {
    return nullptr;
  }
}

ConstantHelper::ConstIntPtr ConstantHelper::Fold(BinarySSA::Operator op,
                                                 const SSAPtr &lhs,
                                                 const SSAPtr &rhs) {
  using Op = BinarySSA::Operator;
  if (auto l = Fold(lhs)) {
    if (auto r = Fold(rhs)) {
      assert(l->type()->IsIdentical(r->type()));
      auto lv = l->value(), rv = r->value();
      auto slv = static_cast<std::int32_t>(lv);
      auto srv = static_cast<std::int32_t>(rv);
      std::uint32_t val;
      switch (op) {
        case Op::Add: val = lv + rv; break;
        case Op::Sub: val = lv - rv; break;
        case Op::Mul: val = lv * rv; break;
        case Op::UDiv: val = lv / rv; break;
        case Op::SDiv: val = slv / srv; break;
        case Op::URem: val = lv % rv; break;
        case Op::SRem: val = slv % srv; break;
        case Op::Equal: val = lv == rv; break;
        case Op::NotEq: val = lv != rv; break;
        case Op::ULess: val = lv < rv; break;
        case Op::SLess: val = slv < srv; break;
        case Op::ULessEq: val = lv <= rv; break;
        case Op::SLessEq: val = slv <= srv; break;
        case Op::UGreat: val = lv > rv; break;
        case Op::SGreat: val = slv > srv; break;
        case Op::UGreatEq: val = lv >= rv; break;
        case Op::SGreatEq: val = slv >= srv; break;
        case Op::And: val = lv & rv; break;
        case Op::Or: val = lv | rv; break;
        case Op::Xor: val = lv ^ rv; break;
        case Op::Shl: val = lv << rv; break;
        case Op::LShr: val = lv >> rv; break;
        case Op::AShr: val = slv >> rv; break;
        default: assert(false);
      }
      auto cint = MakeModule(lhs->logger()).GetInt(val, lhs->type());
      return SSACast<ConstIntSSA>(cint);
    }
  }
  return nullptr;
}

ConstantHelper::ConstIntPtr ConstantHelper::Fold(UnarySSA::Operator op,
                                                 const SSAPtr &opr) {
  using Op = UnarySSA::Operator;
  if (auto v = Fold(opr)) {
    auto val = v->value();
    switch (op) {
      case Op::Neg: val = -val; break;
      case Op::LogicNot: val = !val; break;
      case Op::Not: val = ~val; break;
      default: assert(false);
    }
    auto cint = MakeModule(opr->logger()).GetInt(val, opr->type());
    return SSACast<ConstIntSSA>(cint);
  }
  return nullptr;
}

ConstantHelper::ConstIntPtr ConstantHelper::Fold(const SSAPtr &opr,
                                                 const TypePtr &dest_ty) {
  if (auto v = Fold(opr)) {
    auto val = v->value();
    assert(dest_ty->IsInteger() || dest_ty->IsPointer());
    if (dest_ty->GetSize() == 1) {
      // i8/u8
      val = dest_ty->IsUnsigned() ? static_cast<std::uint8_t>(val)
                                  : static_cast<std::int8_t>(val);
    }
    else if (dest_ty->GetSize() == 4) {
      // i32/u32/ptrs
      val = dest_ty->IsUnsigned() || dest_ty->IsPointer()
                ? static_cast<std::uint32_t>(val)
                : static_cast<std::int32_t>(val);
    }
    else {
      assert(false);
    }
    auto cint = MakeModule(opr->logger()).GetInt(val, dest_ty);
    return SSACast<ConstIntSSA>(cint);
  }
  return nullptr;
}

bool ConstantHelper::IsIdentical(const SSAPtr &val1, const SSAPtr &val2) {
  if (auto c1 = Fold(val1)) {
    if (auto c2 = Fold(val2)) {
      return c1->value() == c2->value();
    }
  }
  return val1 == val2;
}

bool ConstantHelper::IsZero(const mid::SSAPtr &val) {
  if (auto c = Fold(val)) return !c->value();
  return false;
}
