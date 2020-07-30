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

#include "xstl/guard.h"

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
    for (auto &&[label, info] : funcs_) pass->RunOn(label, info.insts);
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

  // generate the specific SSA value
  void GenerateCode(mid::Value &ssa) {
    ssa.GenerateCode(*parent_);
  }

  // generate the specific SSA IR
  void GenerateCode(const mid::SSAPtr &ssa) {
    ssa->GenerateCode(*parent_);
  }

  // get operator pointer from SSA value
  const OprPtr &GetOpr(mid::Value &ssa) {
    auto val = std::any_cast<OprPtr>(&ssa.metadata());
    if (!val) {
      ssa.GenerateCode(*parent_);
      val = std::any_cast<OprPtr>(&ssa.metadata());
      assert(val);
    }
    return *val;
  }

  // get operator pointer from SSA IR
  const OprPtr &GetOpr(const mid::SSAPtr &ssa) {
    return GetOpr(*ssa);
  }

  // enter a new function
  xstl::Guard EnterFunction(const OprPtr &label, LinkageTypes link) {
    return EnterInstSeq(funcs_, label, link);
  }

  // enter a new memory data
  xstl::Guard EnterMemData(const OprPtr &label, LinkageTypes link) {
    return EnterInstSeq(mems_, label, link);
  }

  // add instruction to current instruction sequence
  void AddInst(const InstPtr &inst) {
    cur_seq_->insts.push_back(inst);
  }

  // getters
  // all generated functions
  const InstSeqMap &funcs() const { return funcs_; }
  // all generated memory data
  const InstSeqMap &mems() const { return mems_; }
  // label of current instruction sequence
  const OprPtr &cur_label() const { return *cur_label_; }

 private:
  xstl::Guard EnterInstSeq(InstSeqMap &seqs, const OprPtr &label,
                           LinkageTypes link) {
    auto [it, succ] = seqs.insert({label, {link}});
    assert(succ);
    static_cast<void>(succ);
    auto last_label = cur_label_;
    auto last_seq = cur_seq_;
    cur_label_ = &it->first;
    cur_seq_ = &it->second;
    return xstl::Guard([this, last_label, last_seq] {
      cur_label_ = last_label;
      cur_seq_ = last_seq;
    });
  }

  CodeGen *parent_;
  InstSeqMap funcs_, mems_;
  const OprPtr *cur_label_;
  InstSeqInfo *cur_seq_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_ARCH_INSTGEN_H_
