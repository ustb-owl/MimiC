#ifndef MIMIC_BACK_ASM_MIR_PASS_H_
#define MIMIC_BACK_ASM_MIR_PASS_H_

#include <memory>
#include <list>
#include <utility>

#include "back/asm/mir/mir.h"

namespace mimic::back::asmgen {

// pass interface of machine instructions
class PassInterface {
 public:
  virtual ~PassInterface() = default;

  // run on the specific function (instruction list)
  virtual void RunOn(const OprPtr &func_label, InstPtrList &insts) = 0;
};

using PassPtr = std::unique_ptr<PassInterface>;
using PassPtrList = std::list<PassPtr>;

// create a new pass pointer
template <typename T, typename... Args>
inline std::unique_ptr<T> MakePass(Args &&... args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASS_H_
