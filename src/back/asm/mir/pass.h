#ifndef MIMIC_BACK_ASM_MIR_PASS_H_
#define MIMIC_BACK_ASM_MIR_PASS_H_

#include <memory>
#include <list>

#include "back/asm/mir/mir.h"

namespace mimic::back::asmgen {

// pass interface of machine instructions
class PassInterface {
 public:
  virtual ~PassInterface() = default;

  // run on the specific function (instruction list)
  virtual void RunOn(InstPtrList &insts) = 0;
};

using PassPtr = std::unique_ptr<PassInterface>;
using PassPtrList = std::list<PassPtr>;

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_PASS_H_
