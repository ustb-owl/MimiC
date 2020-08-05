#ifndef MIMIC_OPT_PASSES_HELPER_IRCOPIER_H_
#define MIMIC_OPT_PASSES_HELPER_IRCOPIER_H_

#include <memory>
#include <utility>
#include <unordered_map>

#include "opt/pass.h"

namespace mimic::opt {

// base class for implementing an IR copier, used to copy blocks
// you may want to override the default implementation of the copier
// this can be used when performing function inlining or loop unrolling
class IRCopier : public HelperPass {
 public:
  virtual void RunOn(mid::LoadSSA &ssa) override {
    auto load = CopyFromValue(ssa, nullptr);
    CopyOperand(load, ssa);
  }

  virtual void RunOn(mid::StoreSSA &ssa) override {
    auto store = CopyFromValue(ssa, nullptr, nullptr);
    CopyOperand(store, ssa);
  }

  virtual void RunOn(mid::AccessSSA &ssa) override {
    auto acc = CopyFromValue(ssa, ssa.acc_type(), nullptr, nullptr);
    CopyOperand(acc, ssa);
  }

  virtual void RunOn(mid::BinarySSA &ssa) override {
    auto bin = CopyFromValue(ssa, ssa.op(), nullptr, nullptr);
    CopyOperand(bin, ssa);
  }

  virtual void RunOn(mid::UnarySSA &ssa) override {
    auto una = CopyFromValue(ssa, ssa.op(), nullptr);
    CopyOperand(una, ssa);
  }

  virtual void RunOn(mid::CastSSA &ssa) override {
    auto cast = CopyFromValue(ssa, nullptr);
    CopyOperand(cast, ssa);
  }

  virtual void RunOn(mid::CallSSA &ssa) override {
    auto call = CopyFromValue(ssa, nullptr,
                              mid::SSAPtrList(ssa.size() - 1));
    CopyOperand(call, ssa);
  }

  virtual void RunOn(mid::BranchSSA &ssa) override {
    auto branch = CopyFromValue(ssa, nullptr, nullptr, nullptr);
    CopyOperand(branch, ssa);
  }

  virtual void RunOn(mid::JumpSSA &ssa) override {
    auto jump = CopyFromValue(ssa, nullptr);
    CopyOperand(jump, ssa);
  }

  virtual void RunOn(mid::ReturnSSA &ssa) override {
    auto ret = CopyFromValue(ssa, nullptr);
    CopyOperand(ret, ssa);
  }

  virtual void RunOn(mid::AllocaSSA &ssa) override {
    CopyFromValue(ssa);
  }

  virtual void RunOn(mid::PhiOperandSSA &ssa) override {
    auto phi_opr = CopyFromValue(ssa, nullptr, nullptr);
    CopyOperand(phi_opr, ssa);
  }

  virtual void RunOn(mid::PhiSSA &ssa) override {
    auto phi = CopyFromValue(ssa, mid::SSAPtrList(ssa.size()));
    CopyOperand(phi, ssa);
  }

  virtual void RunOn(mid::SelectSSA &ssa) override {
    auto select = CopyFromValue(ssa, nullptr, nullptr, nullptr);
    CopyOperand(select, ssa);
  }

 protected:
  // clear all copied values
  void ClearCopiedValues() { copied_vals_.clear(); }

  // mark value as copied
  void AddCopiedValue(mid::Value *ptr, const mid::SSAPtr &value) {
    copied_vals_[ptr] = value;
  }

  // get a copy of the specific SSA value
  mid::SSAPtr GetCopy(const mid::SSAPtr &value);

  // make a copy of the specific SSA value
  // returns the pointer of newly created value
  template <typename T, typename... Args>
  inline std::shared_ptr<T> CopyFromValue(T &ssa, Args &&... args) {
    auto val = std::make_shared<T>(std::forward<Args>(args)...);
    val->set_logger(ssa.logger());
    val->set_type(ssa.type());
    copied_vals_[&ssa] = val;
    return val;
  }

  // copy all operands of a SSA user, and set as operands of copied value
  void CopyOperand(const mid::UserPtr &copied, mid::User &user);

 private:
  // all copied values (source -> copied)
  std::unordered_map<mid::Value *, mid::SSAPtr> copied_vals_;
};

}  // namespace mimic::opt

#endif  // MIMIC_OPT_PASSES_HELPER_IRCOPIER_H_
