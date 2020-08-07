#ifndef MIMIC_MID_PASSMAN_H_
#define MIMIC_MID_PASSMAN_H_

#include <string_view>
#include <vector>
#include <type_traits>
#include <memory>
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <cstddef>

#include "opt/pass.h"
#include "opt/stage.h"
#include "mid/usedef.h"

namespace mimic::opt {

// pass information
class PassInfo {
 public:
  using PassNameList = std::vector<std::string_view>;

  PassInfo(PassPtr pass) : pass_(std::move(pass)) {}

  // add required pass by name for current pass
  // all required passes should be run before running current pass
  PassInfo &Requires(std::string_view pass_name) {
    required_passes_.push_back(pass_name);
    return *this;
  }

  // add invalidated pass by name for current pass
  // all invalidated passes should be run again after running current pass
  PassInfo &Invalidates(std::string_view pass_name) {
    invalidated_passes_.push_back(pass_name);
    return *this;
  }

  // setters
  // set minimum optimization level of current pass required
  PassInfo &set_min_opt_level(std::size_t min_opt_level) {
    min_opt_level_ = min_opt_level;
    return *this;
  }
  // set pass stages that current pass should be run at
  PassInfo &set_stages(PassStage stages) {
    stages_ = stages;
    return *this;
  }

  // getters
  const PassPtr &pass() const { return pass_; }
  std::size_t min_opt_level() const { return min_opt_level_; }
  PassStage stages() const { return stages_; }
  const PassNameList &required_passes() const { return required_passes_; }
  const PassNameList &invalidated_passes() const {
    return invalidated_passes_;
  }

 private:
  PassPtr pass_;
  std::size_t min_opt_level_;
  PassStage stages_;
  PassNameList required_passes_, invalidated_passes_;
};

// pass manager for all SSA IR passes
class PassManager {
 public:
  PassManager() : opt_level_(0), stage_(kLastPassStage) {}

  // register a new pass
  template <typename T>
  static PassInfo &RegisterPass(std::string_view name) {
    static_assert(!std::is_base_of_v<HelperPass, T>,
                  "helper pass is unregisterable");
    auto &passes = GetPasses();
    assert(!passes.count(name) && "pass has already been registered");
    return passes.insert({name, PassInfo(std::make_unique<T>())})
        .first->second;
  }

  // get pointer to pass by name
  static const PassPtr &GetPass(std::string_view name) {
    const auto &passes = GetPasses();
    auto it = passes.find(name);
    assert(it != passes.end());
    return it->second.pass();
  }

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
  using PassInfoMap = std::unordered_map<std::string_view, PassInfo>;
  using PassInfoPair = std::pair<std::string_view, const PassInfo *>;
  using PassInfoList = std::vector<PassInfoPair>;
  using PassNameSet = std::unordered_set<std::string_view>;

  // get pass info list
  static PassInfoMap &GetPasses();
  // get passes in specific stage
  PassInfoList GetPasses(PassStage stage) const;
  // run a specific pass, returns true if changed
  bool RunPass(const PassPtr &pass) const;
  // run a specific pass if it's not valid
  // returns true if changed
  bool RunPass(PassNameSet &valid, const PassInfoPair &info) const;
  // run all passes in specific list
  void RunPasses(const PassInfoList &passes) const;

  std::size_t opt_level_;
  PassStage stage_;
  mid::UserPtrList *vars_, *funcs_;
};

// register a pass
#define REGISTER_PASS(cls, name) \
  static PassInfo &pass_##name = PassManager::RegisterPass<cls>(#name)

}  // namespace mimic::opt

#endif  // MIMIC_MID_PASSMAN_H_
