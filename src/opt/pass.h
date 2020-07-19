#ifndef MIMIC_MID_PASS_H_
#define MIMIC_MID_PASS_H_

#include <memory>

#include "mid/ssa.h"

namespace mimic::opt {

// base class of all passes
class PassBase {
 public:
  virtual ~PassBase() = default;

  // return true if is module pass
  virtual bool IsModulePass() const = 0;
  // run on global values in module, return true if there is modification
  virtual bool RunOnModule(mid::UserPtrList &global_vals) = 0;

  // return true if is function pass
  virtual bool IsFunctionPass() const = 0;
  // run on functions, return true if there is modification
  virtual bool RunOnFunction(const mid::UserPtr &func) = 0;

  // return true if is block pass
  virtual bool IsBlockPass() const = 0;
  // run on basic blocks, return true if there is modification
  virtual bool RunOnBlock(const mid::BlockPtr &block) = 0;

  // visitor methods for running on SSA IRs
  virtual void RunOn(mid::LoadSSA &ssa) {}
  virtual void RunOn(mid::StoreSSA &ssa) {}
  virtual void RunOn(mid::AccessSSA &ssa) {}
  virtual void RunOn(mid::BinarySSA &ssa) {}
  virtual void RunOn(mid::UnarySSA &ssa) {}
  virtual void RunOn(mid::CastSSA &ssa) {}
  virtual void RunOn(mid::CallSSA &ssa) {}
  virtual void RunOn(mid::BranchSSA &ssa) {}
  virtual void RunOn(mid::JumpSSA &ssa) {}
  virtual void RunOn(mid::ReturnSSA &ssa) {}
  virtual void RunOn(mid::FunctionSSA &ssa) {}
  virtual void RunOn(mid::GlobalVarSSA &ssa) {}
  virtual void RunOn(mid::AllocaSSA &ssa) {}
  virtual void RunOn(mid::BlockSSA &ssa) {}
  virtual void RunOn(mid::ArgRefSSA &ssa) {}
  virtual void RunOn(mid::ConstIntSSA &ssa) {}
  virtual void RunOn(mid::ConstStrSSA &ssa) {}
  virtual void RunOn(mid::ConstStructSSA &ssa) {}
  virtual void RunOn(mid::ConstArraySSA &ssa) {}
  virtual void RunOn(mid::ConstZeroSSA &ssa) {}
  virtual void RunOn(mid::PhiOperandSSA &ssa) {}
  virtual void RunOn(mid::PhiSSA &ssa) {}
  virtual void RunOn(mid::SelectSSA &ssa) {}
};

// pointer of pass
using PassPtr = std::unique_ptr<PassBase>;

// module pass
class ModulePass : public PassBase {
 public:
  bool IsModulePass() const override final { return true; }
  bool IsFunctionPass() const override final { return false; }
  bool IsBlockPass() const override final { return false; }

  bool RunOnFunction(const mid::UserPtr &funcs) override final {
    return false;
  }
  bool RunOnBlock(const mid::BlockPtr &block) override final {
    return false;
  }
};

// function pass
class FunctionPass : public PassBase {
 public:
  bool IsModulePass() const override final { return false; }
  bool IsFunctionPass() const override final { return true; }
  bool IsBlockPass() const override final { return false; }

  bool RunOnModule(mid::UserPtrList &global_vals) override final {
    return false;
  }
  bool RunOnBlock(const mid::BlockPtr &block) override final {
    return false;
  }
};

// basic block pass
class BlockPass : public PassBase {
 public:
  bool IsModulePass() const override final { return false; }
  bool IsFunctionPass() const override final { return false; }
  bool IsBlockPass() const override final { return true; }

  bool RunOnModule(mid::UserPtrList &global_vals) override final {
    return false;
  }
  bool RunOnFunction(const mid::UserPtr &func) override final {
    return false;
  }
};

// helper pass, for one-time use only, unregisterable
class HelperPass : public PassBase {
 public:
  bool IsModulePass() const override final { return false; }
  bool IsFunctionPass() const override final { return false; }
  bool IsBlockPass() const override final { return false; }

  bool RunOnModule(mid::UserPtrList &global_vals) override final {
    return false;
  }
  bool RunOnFunction(const mid::UserPtr &func) override final {
    return false;
  }
  bool RunOnBlock(const mid::BlockPtr &block) override final {
    return false;
  }
};

}  // namespace mimic::opt

#endif  // MIMIC_MID_PASS_H_
