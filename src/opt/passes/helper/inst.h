#ifndef MIMIC_OPT_PASSES_HELPER_INST_H_
#define MIMIC_OPT_PASSES_HELPER_INST_H_

#include <unordered_map>

#include "opt/pass.h"
#include "opt/passes/helper/cast.h"

namespace mimic::opt {
namespace __impl {

class InstCheckerHelperPass : public HelperPass {
 public:
  bool IsInstruction(mid::Value *val) {
    is_inst_ = false;
    val->RunPass(*this);
    return is_inst_;
  }

  void RunOn(mid::LoadSSA &ssa) override { is_inst_ = true; }
  void RunOn(mid::StoreSSA &ssa) override { is_inst_ = true; }
  void RunOn(mid::AccessSSA &ssa) override { is_inst_ = true; }
  void RunOn(mid::BinarySSA &ssa) override { is_inst_ = true; }
  void RunOn(mid::UnarySSA &ssa) override { is_inst_ = true; }
  void RunOn(mid::CastSSA &ssa) override { is_inst_ = !ssa.IsConst(); }
  void RunOn(mid::CallSSA &ssa) override { is_inst_ = true; }
  void RunOn(mid::BranchSSA &ssa) override { is_inst_ = true; }
  void RunOn(mid::JumpSSA &ssa) override { is_inst_ = true; }
  void RunOn(mid::ReturnSSA &ssa) override { is_inst_ = true; }
  void RunOn(mid::PhiSSA &ssa) override { is_inst_ = true; }
  void RunOn(mid::SelectSSA &ssa) override { is_inst_ = true; }

 private:
  bool is_inst_;
};

}  // namespace __impl

// helper pass for getting parent block of instructions in function
class ParentScanner : public HelperPass {
 public:
  ParentScanner(const mid::UserPtr &func) { func->RunPass(*this); }
  ParentScanner(mid::FunctionSSA *func) { func->RunPass(*this); }

  void RunOn(mid::FunctionSSA &ssa) override;

  // get parent block of specific value
  mid::BlockSSA *GetParent(mid::Value *val);

 private:
  std::unordered_map<mid::Value *, mid::BlockSSA *> parent_map_;
};


// check if value is a instruction
inline bool IsInstruction(const mid::Value *val) {
  __impl::InstCheckerHelperPass helper;
  return helper.IsInstruction(const_cast<mid::Value *>(val));
}
inline bool IsInstruction(const mid::Value &val) {
  return IsInstruction(&val);
}
inline bool IsInstruction(const mid::SSAPtr &val) {
  return IsInstruction(val.get());
}

// check if instruction is dead
inline bool IsInstDead(const mid::Value *inst) {
  if (!IsInstruction(inst)) return false;
  if (!inst->uses().empty()) return false;
  if (IsSSA<mid::StoreSSA>(inst) || IsSSA<mid::CallSSA>(inst) ||
      IsSSA<mid::BranchSSA>(inst) || IsSSA<mid::JumpSSA>(inst) ||
      IsSSA<mid::ReturnSSA>(inst)) {
    return false;
  }
  return true;
}
inline bool IsInstDead(const mid::Value &val) {
  return IsInstDead(&val);
}
inline bool IsInstDead(const mid::SSAPtr &val) {
  return IsInstDead(val.get());
}

// dynamic cast a SSA value to instruction
inline const mid::User *InstDynCast(const mid::Value *ssa) {
  return IsInstruction(ssa) ? static_cast<const mid::User *>(ssa) : nullptr;
}
inline mid::User *InstDynCast(mid::Value *ssa) {
  return IsInstruction(ssa) ? static_cast<mid::User *>(ssa) : nullptr;
}
inline mid::UserPtr InstDynCast(const mid::SSAPtr &ssa) {
  return IsInstruction(ssa) ? std::static_pointer_cast<mid::User>(ssa)
                            : nullptr;
}

// static cast a SSA value to instruction
inline const mid::User *InstCast(const mid::Value *ssa) {
  assert(IsInstruction(ssa) && "InstCast failed");
  return static_cast<const mid::User *>(ssa);
}
inline mid::User *InstCast(mid::Value *ssa) {
  assert(IsInstruction(ssa) && "InstCast failed");
  return static_cast<mid::User *>(ssa);
}
inline mid::UserPtr InstCast(const mid::SSAPtr &ssa) {
  assert(IsInstruction(ssa) && "InstCast failed");
  return std::static_pointer_cast<mid::User>(ssa);
}

}  // namespace mimic::opt

#endif  // MIMIC_OPT_PASSES_HELPER_INST_H_
