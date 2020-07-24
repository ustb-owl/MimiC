#ifndef MIMIC_BACK_ASM_ARCH_INSTGEN_H_
#define MIMIC_BACK_ASM_ARCH_INSTGEN_H_

#include <ostream>
#include <unordered_map>
#include <vector>
#include <utility>
#include <memory>
#include <cstdint>
#include <cassert>

#include "mid/ssa.h"
#include "back/asm/mir/mir.h"

namespace mimic::back::asmgen {

// base class of all machine instruction generator
class InstGenBase {
 public:
  virtual ~InstGenBase() = default;

  virtual OprPtr GenerateOn(mid::LoadSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::StoreSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::AccessSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::BinarySSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::UnarySSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::CastSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::CallSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::BranchSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::JumpSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::ReturnSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::FunctionSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::GlobalVarSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::AllocaSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::BlockSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::ArgRefSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::ConstIntSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::ConstStrSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::ConstStructSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::ConstArraySSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::ConstZeroSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::SelectSSA &ssa) = 0;
  virtual OprPtr GenerateOn(mid::UndefSSA &ssa) = 0;

  // dump to assembly
  virtual void Dump(std::ostream &os) const = 0;

  // run machine level passes on all functions
  void RunPasses(/* pass manager interface */) {
    // TODO
  }

 protected:
  // information of function
  struct FuncInfo {
    // set if function is global
    bool is_global;
    // instruction list of function
    InstPtrList insts;
  };

  // information of memory data
  struct MemInfo {
    // set if memory data is global
    bool is_global;
    // data in bytes
    std::vector<std::uint8_t> data;
  };

  // enter a new function
  void EnterFunction(const OprPtr &label, bool is_global) {
    auto [it, succ] = funcs_.insert({label, {is_global}});
    assert(succ);
    static_cast<void>(succ);
    cur_func_ = &it->second;
  }

  // add instruction to current function
  void AddInst(const InstPtr &inst) {
    cur_func_->insts.push_back(inst);
  }

  // create a new memory data
  void CreateMem(const OprPtr &label, MemInfo mem_info) {
    auto succ = mems_.insert({label, std::move(mem_info)}).second;
    assert(succ);
    static_cast<void>(succ);
  }

  // getters
  // all generated functions
  const std::unordered_map<OprPtr, FuncInfo> &funcs() const {
    return funcs_;
  }
  // all generated memory data
  const std::unordered_map<OprPtr, MemInfo> &mems() const { return mems_; }

 private:
  std::unordered_map<OprPtr, FuncInfo> funcs_;
  std::unordered_map<OprPtr, MemInfo> mems_;
  FuncInfo *cur_func_;
};

// pointer to instruction generator
using InstGen = std::unique_ptr<InstGenBase>;

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_ARCH_INSTGEN_H_
