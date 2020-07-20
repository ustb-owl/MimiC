#ifndef MIMIC_PASS_HELPER_UTILS_H_
#define MIMIC_PASS_HELPER_UTILS_H_

#include <type_traits>
#include <memory>
#include <cassert>

#include "opt/pass.h"

namespace mimic::opt {

// helper for checking type of SSA
template <typename T>
class IsSSAHelperPass : public HelperPass {
 public:
  static_assert(std::is_base_of_v<mid::Value, T> &&
                    !std::is_same_v<mid::Value, T> &&
                    !std::is_same_v<mid::User, T>,
                "SSA type required");

  bool Check(mid::Value *ssa) {
    yes_ = false;
    ssa->RunPass(*this);
    return yes_;
  }

  void RunOn(T &ssa) override { yes_ = true; }

 private:
  bool yes_;
};

// check if value is specific type
template <typename T>
inline bool IsSSA(mid::Value *ssa) {
  IsSSAHelperPass<T> helper;
  return helper.Check(ssa);
}
template <typename T>
inline bool IsSSA(mid::Value &ssa) {
  return IsSSA<T>(&ssa);
}
template <typename T>
inline bool IsSSA(const mid::SSAPtr &ssa) {
  return IsSSA<T>(ssa.get());
}

// perform dynamic casting on SSA values
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

#endif  // MIMIC_PASS_HELPER_UTILS_H_
