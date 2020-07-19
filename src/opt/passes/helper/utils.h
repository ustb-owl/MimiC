#ifndef MIMIC_PASS_HELPER_UTILS_H_
#define MIMIC_PASS_HELPER_UTILS_H_

#include <type_traits>

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

}  // namespace mimic::opt

#endif  // MIMIC_PASS_HELPER_UTILS_H_
