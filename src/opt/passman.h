#ifndef MIMIC_MID_PASSMAN_H_
#define MIMIC_MID_PASSMAN_H_

#include <string_view>
#include <ostream>
#include <list>
#include <memory>
#include <type_traits>
#include <cstddef>

#include "opt/pass.h"
#include "mid/usedef.h"

namespace mimic::opt {

// pass information
class PassInfo {
 public:
  PassInfo(std::string_view name, PassPtr pass, std::size_t min_opt_level)
      : name_(name), pass_(std::move(pass)),
        min_opt_level_(min_opt_level) {}
  virtual ~PassInfo() = default;

  // getters
  std::string_view name() const { return name_; }
  const PassPtr &pass() const { return pass_; }
  std::size_t min_opt_level() const { return min_opt_level_; }

 private:
  std::string_view name_;
  PassPtr pass_;
  std::size_t min_opt_level_;
};

// pass manager for all SSA IR passes
class PassManager {
 public:
  PassManager() : opt_level_(0) {}

  // register a new pass
  static void RegisterPass(PassInfo *info);

  // run all passes on specific module
  void RunPasses() const;
  // show info of passes
  void ShowInfo(std::ostream &os) const;

  // setters
  void set_opt_level(std::size_t opt_level) { opt_level_ = opt_level; }
  void set_vars(mid::UserPtrList *vars) { vars_ = vars; }
  void set_funcs(mid::UserPtrList *funcs) { funcs_ = funcs; }

  // getters
  std::size_t opt_level() const { return opt_level_; }

 private:
  // get pass info list
  static std::list<PassInfo *> &GetPasses();

  std::size_t opt_level_;
  mid::UserPtrList *vars_, *funcs_;
};

// helper class for registering a pass
template <typename T>
class RegisterPass : public PassInfo {
 public:
  RegisterPass(std::string_view name, std::size_t min_opt_level)
      : PassInfo(name, std::make_unique<T>(), min_opt_level) {
    PassManager::RegisterPass(this);
  }
};

// register a pass
#define REGISTER_PASS(cls, name, min_opt_level) \
  static_assert(!std::is_base_of_v<HelperPass, cls>,         \
                "helper pass is unregisterable");            \
  static RegisterPass<cls> pass_##name(#name, min_opt_level)

}  // namespace mimic::opt

#endif  // MIMIC_MID_PASSMAN_H_
