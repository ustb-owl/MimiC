#include <string>
#include <unordered_set>
#include <vector>
#include <cassert>
#include <cstddef>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/helper/cast.h"
#include "opt/helper/ircopier.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

// helper class for copying constant array
class ConstArrayCopyier : public IRCopier {
 public:
  using IRCopier::RunOn;

  SSAPtr Copy(const SSAPtr &val) {
    return GetCopy(val);
  }

  void RunOn(ConstArraySSA &ssa) override {
    auto carr = CopyFromValue(ssa, SSAPtrList(ssa.size()));
    CopyOperand(carr, ssa);
  }

  void RunOn(ConstIntSSA &ssa) override {
    auto cint = std::make_shared<ConstIntSSA>(ssa.value());
    cint->set_logger(ssa.logger());
    cint->set_type(ssa.type());
    AddCopiedValue(&ssa, cint);
  }
};


/*
  this pass will promote all local arrays that has only been
  stored once (with a constant value) to global array constant
*/
class LocalArrayPromotionPass : public ModulePass {
 public:
  LocalArrayPromotionPass() : global_id_(0) {}

  bool RunOnModule(UserPtrList &global_vals) override {
    // check if is global variable list
    // TODO: a little tricky
    if (!gvar_list_) {
      gvar_list_ = &global_vals;
      return false;
    }
    // traverse all functions
    bool changed = false;
    for (const auto &func : global_vals) {
      // skip declarations
      auto func_ptr = SSACast<FunctionSSA>(func.get());
      if (func_ptr->is_decl()) continue;
      auto entry = SSACast<BlockSSA>(func_ptr->entry().get());
      // remember all instructions first
      for (const auto &i : entry->insts()) {
        entry_insts_.insert(i.get());
      }
      // traverse all allocas in entry block
      for (const auto &i : entry->insts()) {
        i->RunPass(*this);
      }
      entry_insts_.clear();
      // handle removed stores
      if (!removed_stores_.empty()) {
        for (const auto &store : removed_stores_) {
          entry->insts().remove_if(
              [store](const SSAPtr &i) { return i.get() == store; });
        }
        removed_stores_.clear();
        // updated 'changed' flag
        changed = true;
      }
    }
    return changed;
  }

  void Initialize() override {
    gvar_list_ = nullptr;
  }

  void RunOn(AllocaSSA &ssa) override {
    auto arr_ty = ssa.type()->GetDerefedType();
    if (!arr_ty->IsArray()) return;
    // mark as can be promoted at first
    can_prom_ = true;
    cur_alloca_ = &ssa;
    key_store_ = nullptr;
    // traverse all users, try to prove current alloca can not be promoted
    for (const auto &i : ssa.uses()) {
      i->user()->RunPass(*this);
      if (!can_prom_) break;
    }
    // key store instruction must be in entry block
    if (can_prom_ && entry_insts_.count(key_store_)) {
      assert(gvar_list_);
      // make a copy of stored value
      auto value = key_store_->value();
      if (auto carr = SSADynCast<ConstArraySSA>(value)) {
        value = ConstArrayCopyier().Copy(carr);
      }
      // create a new global array constant
      auto name = "__garr" + std::to_string(global_id_++);
      auto mod = MakeModule(ssa.logger());
      auto gvar = mod.CreateGlobalVar(LinkageTypes::Internal, false, name,
                                      arr_ty, value);
      gvar_list_->push_back(gvar);
      // replace with created value
      ssa.ReplaceBy(gvar);
      // mark key store instruction as removed
      removed_stores_.push_back(key_store_);
    }
  }

  void RunOn(StoreSSA &ssa) override {
    if (ssa.ptr().get() == cur_alloca_ && ssa.value()->IsConst() &&
        !key_store_) {
      // the only constant store to local array
      key_store_ = &ssa;
    }
    else {
      // multiple store instructions exist, can not be promoted
      can_prom_ = false;
    }
  }

  void RunOn(AccessSSA &ssa) override {
    // try to find store instructions
    for (const auto &i : ssa.uses()) {
      i->user()->RunPass(*this);
      if (!can_prom_) break;
    }
  }

  void RunOn(CastSSA &ssa) override {
    // try to find store instructions
    for (const auto &i : ssa.uses()) {
      i->user()->RunPass(*this);
      if (!can_prom_) break;
    }
  }

  void RunOn(CallSSA &ssa) override {
    // passed pointer of alloca into function call
    // just mark as can not be promoted
    can_prom_ = false;
  }

 private:
  // all instructions in entry block
  std::unordered_set<Value *> entry_insts_;
  // current alloca can be promoted
  bool can_prom_;
  // current alloca instruction & last visited SSA value
  Value *cur_alloca_;
  // the key store instruction
  StoreSSA *key_store_;
  // store instructions that should be removed
  std::vector<StoreSSA *> removed_stores_;
  // list of global variables
  UserPtrList *gvar_list_;
  // id of global variable, used to name generated global arrays
  // NOTE: this property will not be reset between pass calls
  std::size_t global_id_;
};

}  // namespace


// register current pass
REGISTER_PASS(LocalArrayPromotionPass, local_prom)
    .set_min_opt_level(2)
    .set_stages(PassStage::Opt);

