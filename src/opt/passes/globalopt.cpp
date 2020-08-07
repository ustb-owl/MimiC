#include <unordered_map>
#include <unordered_set>
#include <cstddef>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/helper/cast.h"
#include "mid/module.h"

#include "xstl/guard.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  this pass will scan all global variables
  and mark variables that have only been read as constants
*/
class GlobalOptPass : public ModulePass {
 public:
  GlobalOptPass() {}

  bool RunOnModule(UserPtrList &global_vals) override {
    // map for recording whether a global variable is a constant
    std::unordered_map<GlobalVarSSA *, bool> gvars;
    // scan all global variables
    for (const auto &i : global_vals) {
      if (auto gvar = SSADynCast<GlobalVarSSA>(i.get())) {
        // skip constants and external values
        if (!gvar->is_var() || !IsInternal(gvar->link())) continue;
        // mark as constant at first
        not_a_const_ = false;
        last_visited_ = gvar;
        // traverse all users, try to prove it's not a constant
        for (const auto &i : gvar->uses()) {
          i->user()->RunPass(*this);
          if (not_a_const_) break;
        }
        gvars[gvar] = !not_a_const_;
      }
    }
    // update all marked global variables
    bool changed = false;
    for (const auto &[gvar, is_const] : gvars) {
      if (is_const) {
        gvar->set_is_var(false);
        if (!changed) changed = true;
      }
    }
    return changed;
  }

  void RunOn(StoreSSA &ssa) override {
    // stored to a variable, so it's not a constant
    if (ssa.ptr().get() == last_visited_) not_a_const_ = true;
  }

  void RunOn(AccessSSA &ssa) override {
    auto last = UpdateLast(ssa);
    // traverse all users, try to find a store instruction
    for (const auto &i : ssa.uses()) {
      i->user()->RunPass(*this);
      if (not_a_const_) break;
    }
  }

  void RunOn(CastSSA &ssa) override {
    auto last = UpdateLast(ssa);
    // traverse all users, try to find a store instruction
    for (const auto &i : ssa.uses()) {
      i->user()->RunPass(*this);
      if (not_a_const_) break;
    }
  }

  void RunOn(CallSSA &ssa) override {
    // global variable has been passed into a function call
    // try to find out if the argument has ever been written
    for (std::size_t i = 1; i < ssa.size(); ++i) {
      if (ssa[i].value().get() != last_visited_) continue;
      // get callee & check if is internal
      auto callee = SSACast<FunctionSSA>(ssa.callee().get());
      if (!IsInternal(callee->link())) {
        // argument of external function, mark as not a constant
        not_a_const_ = true;
        return;
      }
      // visit all users
      auto last = UpdateLast(ssa);
      const auto &arg = callee->args()[i - 1];
      for (const auto &i : arg->uses()) {
        i->user()->RunPass(*this);
        if (not_a_const_) break;
      }
      return;
    }
  }

 private:
  bool IsInternal(LinkageTypes link) {
    return link == LinkageTypes::Internal || link == LinkageTypes::Inline;
  }

  xstl::Guard UpdateLast(Value &val) {
    auto last_lv = last_visited_;
    last_visited_ = &val;
    return xstl::Guard([this, &last_lv] { last_visited_ = last_lv; });
  }

  // current value is not a constant
  bool not_a_const_;
  // last visited ssa value
  Value *last_visited_;
};

}  // namespace

// register current pass
REGISTER_PASS(GlobalOptPass, global_opt)
    .set_min_opt_level(2)
    .set_stages(PassStage::Opt);
