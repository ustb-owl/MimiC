#ifndef MIMIC_OPT_HELPER_UTILS_H_
#define MIMIC_OPT_HELPER_UTILS_H_

#include <type_traits>
#include <memory>
#include <cassert>

#include "opt/pass.h"

/*
  Runtime type checking of SSA values

  This utility is implemented by using visitor pattern and template,
  although it's deviated from the original intention of visitor pattern.
*/

namespace mimic::opt {
namespace __impl {

// all supported SSA types
#define ALL_SSA(e) \
  e(Load) e(Store) e(Access) e(Binary) e(Unary) e(Cast) e(Call) \
  e(Branch) e(Jump) e(Return) e(Function) e(GlobalVar) \
  e(Alloca) e(Block) e(ArgRef) e(ConstInt) e(ConstStr) \
  e(ConstStruct) e(ConstArray) e(ConstZero) \
  e(PhiOperand) e(Phi) e(Select) e(Undef)
// expand to enum definition
#define EXPAND_ENUM(ssa) ssa,
// expand to method declaration
#define EXPAND_METHOD(ssa) void RunOn(mid::ssa##SSA &) override;
// expand to static type checking
#define EXPAND_TYPE_CHECKING(ssa)                        \
  else if constexpr (std::is_same_v<mid::ssa##SSA, T>) { \
    return type_ == SSAType::ssa;                        \
  }

// helper for static type checking
template <typename ...>
constexpr std::false_type always_false;

// helper for checking type of SSA
// NOTE: thread unsafe!
class IsSSAHelperPass : public HelperPass {
 public:
  template <typename T>
  static bool Check(mid::Value *ssa) {
    static_assert(std::is_base_of_v<mid::Value, T> &&
                      !std::is_same_v<mid::Value, T> &&
                      !std::is_same_v<mid::User, T>,
                  "SSA type required");
    type_ = SSAType::None;
    ssa->RunPass(GetInstance());
    if constexpr (always_false<T>) {}
    ALL_SSA(EXPAND_TYPE_CHECKING)
    else static_assert(always_false<T>);
  }

  ALL_SSA(EXPAND_METHOD);

 private:
  // type specifier of all SSAs
  enum class SSAType { None, ALL_SSA(EXPAND_ENUM) };

  static IsSSAHelperPass &GetInstance();
  static SSAType type_;
};

#undef ALL_SSA
#undef EXPAND_ENUM
#undef EXPAND_METHOD
#undef EXPAND_TYPE_CHECKING

}  // namespace __impl

// check if value is specific type
template <typename T>
inline bool IsSSA(const mid::Value *ssa) {
  return __impl::IsSSAHelperPass::Check<T>(const_cast<mid::Value *>(ssa));
}
template <typename T>
inline bool IsSSA(const mid::Value &ssa) {
  return IsSSA<T>(&ssa);
}
template <typename T>
inline bool IsSSA(const mid::SSAPtr &ssa) {
  return IsSSA<T>(ssa.get());
}

// perform dynamic casting on SSA values
template <typename T>
inline const T *SSADynCast(const mid::Value *ssa) {
  return IsSSA<T>(ssa) ? static_cast<const T *>(ssa) : nullptr;
}
template <typename T>
inline T *SSADynCast(mid::Value *ssa) {
  return IsSSA<T>(ssa) ? static_cast<T *>(ssa) : nullptr;
}
template <typename T>
inline std::shared_ptr<T> SSADynCast(const mid::SSAPtr &ssa) {
  return IsSSA<T>(ssa) ? std::static_pointer_cast<T>(ssa) : nullptr;
}

// perform static casting on SSA values
template <typename T>
inline const T *SSACast(const mid::Value *ssa) {
  assert(IsSSA<T>(ssa) && "SSACast failed");
  return static_cast<const T *>(ssa);
}
template <typename T>
inline T *SSACast(mid::Value *ssa) {
  assert(IsSSA<T>(ssa) && "SSACast failed");
  return static_cast<T *>(ssa);
}
template <typename T>
inline std::shared_ptr<T> SSACast(const mid::SSAPtr &ssa) {
  assert(IsSSA<T>(ssa) && "SSACast failed");
  return std::static_pointer_cast<T>(ssa);
}

}  // namespace mimic::opt

#endif  // MIMIC_OPT_HELPER_UTILS_H_
