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
#include "back/asm/mir/pass.h"

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

  // run a machine level pass on all functions
  void RunPass(const PassPtr &pass) {
    for (auto &&[_, info] : funcs_) pass->RunOn(info.insts);
  }

  // setters
  void set_parent(CodeGen *parent) { parent_ = parent; }

 protected:
  // linkage types for backend
  enum class LinkageTypes {
    Internal, External, Ctor, Dtor,
  };

  // information of instruction sequence
  struct InstSeqInfo {
    LinkageTypes link;
    InstPtrList insts;
  };

  // map of instruction sequence
  using InstSeqMap = std::unordered_map<OprPtr, InstSeqInfo>;

  // get operator pointer from SSA IR
  const OprPtr &GetOpr(const mid::SSAPtr &ssa) {
    auto val = std::any_cast<OprPtr>(&ssa->metadata());
    if (!val) {
      ssa->GenerateCode(*parent_);
      val = std::any_cast<OprPtr>(&ssa->metadata());
      assert(val);
    }
    return *val;
  }

  // enter a new function
  void EnterFunction(const OprPtr &label, LinkageTypes link) {
    auto [it, succ] = funcs_.insert({label, {link}});
    assert(succ);
    static_cast<void>(succ);
    cur_func_ = &it->second;
  }

  // add instruction to current function
  void AddInst(const InstPtr &inst) {
    cur_func_->insts.push_back(inst);
  }

  // create a new memory data
  void CreateMem(const OprPtr &label, InstSeqInfo mem_info) {
    auto succ = mems_.insert({label, std::move(mem_info)}).second;
    assert(succ);
    static_cast<void>(succ);
  }

  // getters
  // all generated functions
  const InstSeqMap &funcs() const { return funcs_; }
  // all generated memory data
  const InstSeqMap &mems() const { return mems_; }

 private:
  CodeGen *parent_;
  InstSeqMap funcs_, mems_;
  InstSeqInfo *cur_func_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_ARCH_INSTGEN_H_
