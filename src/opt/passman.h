#ifndef MIMIC_MID_PASSMAN_H_
#define MIMIC_MID_PASSMAN_H_

#include <string_view>
#include <ostream>
#include <list>
#include <memory>
#include <type_traits>
#include <cstddef>

#include "opt/pass.h"
#include "opt/stage.h"
#include "mid/usedef.h"

namespace mimic::opt {

// pass information
class PassInfo {
 public:
  PassInfo(std::string_view name, PassPtr pass, std::size_t min_opt_level,
           PassStage stage)
      : name_(name), pass_(std::move(pass)),
        min_opt_level_(min_opt_level), stage_(stage) {}
  virtual ~PassInfo() = default;

  // getters
  std::string_view name() const { return name_; }
  const PassPtr &pass() const { return pass_; }
  std::size_t min_opt_level() const { return min_opt_level_; }
  PassStage stage() const { return stage_; }

 private:
  std::string_view name_;
  PassPtr pass_;
  std::size_t min_opt_level_;
  PassStage stage_;
};

// pass manager for all SSA IR passes
class PassManager {
 public:
  PassManager() : opt_level_(0), stage_(kLastPassStage) {}

  // register a new pass
  static void RegisterPass(PassInfo *info);

  // run passes on current module
  void RunPasses() const;
  // show info of passes
  void ShowInfo(std::ostream &os) const;

  // setters
  void set_opt_level(std::size_t opt_level) { opt_level_ = opt_level; }
  void set_stage(PassStage stage) { stage_ = stage; }
  void set_vars(mid::UserPtrList *vars) { vars_ = vars; }
  void set_funcs(mid::UserPtrList *funcs) { funcs_ = funcs; }

  // getters
  std::size_t opt_level() const { return opt_level_; }
  bool is_stage_last() const { return stage_ == kLastPassStage; }

 private:
  // get pass info list
  static std::list<PassInfo *> &GetPasses();
  // get passes in specific stage
  std::list<PassInfo *> GetPasses(PassStage stage) const;
  // run all passes in specific list
  void RunPasses(const std::list<PassInfo *> &passes) const;

  std::size_t opt_level_;
  PassStage stage_;
  mid::UserPtrList *vars_, *funcs_;
};

// helper class for registering a pass
template <typename T>
class RegisterPass : public PassInfo {
 public:
  RegisterPass(std::string_view name, std::size_t min_opt_level,
               PassStage stage)
      : PassInfo(name, std::make_unique<T>(), min_opt_level, stage) {
    PassManager::RegisterPass(this);
  }
};

// register a pass
#define REGISTER_PASS(cls, name, min_opt_level, stage)    \
  static_assert(!std::is_base_of_v<HelperPass, cls>,      \
                "helper pass is unregisterable");         \
  static_assert(min_opt_level >= 0 && min_opt_level <= 3, \
                "invalid optimization level");            \
  static RegisterPass<cls> pass_##name(#name, min_opt_level, stage)

}  // namespace mimic::opt

#endif  // MIMIC_MID_PASSMAN_H_
