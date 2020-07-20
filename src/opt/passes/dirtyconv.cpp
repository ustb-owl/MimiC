#include <string_view>
#include <string>

#include "opt/pass.h"
#include "opt/passman.h"
#include "mid/module.h"
#include "define/type.h"

using namespace mimic::mid;
using namespace mimic::opt;
using namespace mimic::define;

namespace {

/*
  some dirty conversion for SysY programming language
  this pass will:
    1.  perform conversion on some function calls
        e.g. starttime() -> _sysy_starttime(line_num)
    2.  mark all function definitions and global variables
        as internal, except 'main' function
*/
class DirtyConversionPass : public ModulePass {
 public:
  DirtyConversionPass() {}

  bool RunOnModule(UserPtrList &global_vals) override {
    changed_ = false;
    in_func_ = false;
    start_time_ = nullptr;
    stop_time_ = nullptr;
    // traverse all global values
    for (const auto &i : global_vals) {
      i->RunPass(*this);
    }
    // insert function declarations if necessary
    if (start_time_) global_vals.push_front(start_time_);
    if (stop_time_) global_vals.push_front(stop_time_);
    return changed_;
  }

  void RunOn(CallSSA &ssa) override {
    // check if is target function call
    if (ssa.size() != 1) return;
    ssa[0].value()->RunPass(*this);
    if (name_ != "starttime" && name_ != "stoptime") return;
    // create temp module
    auto mod = MakeModule(ssa.logger());
    // create function declaration if does not exist
    auto &decl = name_ == "starttime" ? start_time_ : stop_time_;
    if (!decl) {
      using namespace std::string_literals;
      auto name = "_sysy_"s + std::string(name_);
      TypePtrList args = {MakePrimType(PrimType::Type::Int32, false)};
      auto ret = MakePrimType(PrimType::Type::Void, false);
      auto type = std::make_shared<FuncType>(std::move(args),
                                             std::move(ret), false);
      decl = mod.CreateFunction(LinkageTypes::External, name,
                                std::move(type));
    }
    // modify current call instruction
    ssa[0].set_value(decl);
    ssa.AddValue(mod.GetInt32(ssa.logger()->line_pos()));
    if (!changed_) changed_ = true;
  }

  void RunOn(BlockSSA &ssa) override {
    // scan all instructions
    for (const auto &i : ssa.insts()) i->RunPass(*this);
  }

  void RunOn(FunctionSSA &ssa) override {
    if (in_func_) {
      // log function name
      name_ = ssa.name();
    }
    else if (!ssa.empty()) {
      // try to set as internal
      if (ssa.link() == LinkageTypes::External && ssa.name() != "main") {
        ssa.set_link(LinkageTypes::Internal);
      }
      // scan all basic blocks
      in_func_ = true;
      for (const auto &i : ssa) i.value()->RunPass(*this);
      in_func_ = false;
    }
  }

  void RunOn(GlobalVarSSA &ssa) override {
    if (!in_func_ && ssa.link() == LinkageTypes::External) {
      ssa.set_link(LinkageTypes::Internal);
    }
  }

 private:
  bool changed_;
  // set if is in function
  bool in_func_;
  // name of last function
  std::string_view name_;
  // global function declarations
  UserPtr start_time_, stop_time_;
};

}  // namespace

REGISTER_PASS(DirtyConversionPass, dirty_conv, 0, PassStage::PreOpt);
