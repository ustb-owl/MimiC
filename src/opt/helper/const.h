#ifndef MIMIC_OPT_HELPER_CONST_H_
#define MIMIC_OPT_HELPER_CONST_H_

#include <memory>

#include "mid/ssa.h"

namespace mimic::opt {

class ConstantHelper {
 public:
  using ConstIntPtr = std::shared_ptr<mid::ConstIntSSA>;

  // fold constant instruction with int/ptr type if possible
  // returns nullptr if failed
  static ConstIntPtr Fold(const mid::SSAPtr &val);

  // fold binary operation if possible
  // lhs & rhs must be constant
  static ConstIntPtr Fold(mid::BinarySSA::Operator op,
                          const mid::SSAPtr &lhs, const mid::SSAPtr &rhs);

  // fold unary operation if possible
  static ConstIntPtr Fold(mid::UnarySSA::Operator op,
                          const mid::SSAPtr &opr);

  // fold type casting if possible
  static ConstIntPtr Fold(const mid::SSAPtr &opr,
                          const define::TypePtr &dest_ty);

  // check if two values are identical
  // if values are both constant, compare their actual values
  // otherwise, perform pointer comparison
  static bool IsIdentical(const mid::SSAPtr &val1, const mid::SSAPtr &val2);

  // check if integer value equals to constant zero
  // returns false if value is not an integer/pointer, or value is not zero
  static bool IsZero(const mid::SSAPtr &val);

 private:
  ConstantHelper() {}
};


}  // namespace mimic::opt

#endif  // MIMIC_OPT_HELPER_CONST_H_
