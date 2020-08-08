#include "opt/passman.h"

#include <iomanip>
#include <cassert>

#include "opt/helper/cast.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

std::ostream &operator<<(std::ostream &os, PassStage stage) {
  bool is_first = true;
  for (std::size_t i = 0; i < kPassStageCount; ++i) {
    auto cur = stage & static_cast<PassStage>(1 << i);
    if (cur != PassStage::None) {
      if (!is_first) {
        os << " | ";
      }
      else {
        is_first = false;
      }
      switch (cur) {
        case PassStage::PreOpt: os << "PreOpt"; break;
        case PassStage::Promote: os << "Promote"; break;
        case PassStage::Opt: os << "Opt"; break;
        case PassStage::Demote: os << "Demote"; break;
        case PassStage::PostOpt: os << "PostOpt"; break;
        default:;
      }
    }
  }
  return os;
}

}  // namespace

PassInfo &PassInfo::Requires(std::string_view pass_name) {
  required_passes_.insert(pass_name);
  PassManager::RequiredBy(pass_name, this);
  return *this;
}

PassInfo &PassInfo::Invalidates(std::string_view pass_name) {
  invalidated_passes_.insert(pass_name);
  return *this;
}

PassManager::PassInfoMap &PassManager::GetPasses() {
  static PassInfoMap passes;
  return passes;
}

PassManager::RequirementMap &PassManager::GetRequiredBy() {
  static RequirementMap required_by;
  return required_by;
}

PassManager::PassPtrList PassManager::GetPasses(PassStage stage) const {
  PassPtrList passes;
  // filter all passes that can be run at current stage & opt_level
  for (const auto &[_, info] : GetPasses()) {
    if (!info.is_analysis() && opt_level_ >= info.min_opt_level() &&
        (info.stages() & stage) != PassStage::None) {
      passes.push_back(&info);
    }
  }
  return passes;
}

bool PassManager::RunPass(const PassPtr &pass) const {
  bool changed = false;
  // initialize
  pass->Initialize();
  // handle by pass type
  if (pass->IsModulePass()) {
    // run on global values
    if (pass->RunOnModule(*vars_)) changed = true;
    if (pass->RunOnModule(*funcs_)) changed = true;
    pass->CleanUp();
  }
  else if (pass->IsFunctionPass()) {
    // traverse all functions
    for (const auto &i : *funcs_) {
      auto func = SSACast<FunctionSSA>(i);
      if (pass->RunOnFunction(func)) changed = true;
      pass->CleanUp();
    }
  }
  else {
    assert(pass->IsBlockPass());
    // traverse all basic blocks
    for (const auto &func : *funcs_) {
      for (const auto &i : *func) {
        auto blk = SSACast<BlockSSA>(i.value());
        if (pass->RunOnBlock(blk)) changed = true;
        pass->CleanUp();
      }
    }
  }
  return changed;
}

bool PassManager::RunPass(PassPtrSet &valid, const PassInfo *info) const {
  bool changed = false;
  if (!valid.insert(info).second) return changed;
  // check dependencies, run required passes first
  for (const auto &name : info->required_passes()) {
    const auto &passes = GetPasses();
    auto it = passes.find(name);
    assert(it != passes.end());
    if (!valid.count(&it->second)) {
      changed |= RunPass(valid, &it->second);
    }
  }
  // run current pass
  if (RunPass(info->pass())) {
    changed = true;
    // invalidate passes
    InvalidatePass(valid, info);
  }
  return changed;
}

void PassManager::InvalidatePass(PassPtrSet &valid,
                                 const PassInfo *info) const {
  for (const auto &name : info->invalidated_passes()) {
    // get pointer to pass
    const auto &passes = GetPasses();
    auto it = passes.find(name);
    assert(it != passes.end());
    // erase from valid set
    if (!valid.erase(&it->second)) continue;
    // invalidate all passes that required current pass
    for (const auto &child : GetRequiredBy()[name]) {
      valid.erase(child);
      InvalidatePass(valid, child);
    }
  }
}

void PassManager::RunPasses(const PassPtrList &passes) const {
  bool changed = true;
  PassPtrSet valid;
  // run until nothing changes
  while (changed) {
    changed = false;
    valid.clear();
    // traverse all passes
    for (const auto &info : passes) {
      changed |= RunPass(valid, info);
    }
  }
}

void PassManager::RunPasses() const {
  // traverse all stages
  for (auto i = kFirstPassStage; i <= stage_; ++i) {
    // get passes in current stage
    auto passes = GetPasses(i);
    // run on current stage
    RunPasses(passes);
  }
}

void PassManager::ShowInfo(std::ostream &os) const {
  // display optimization level & stage
  os << "current optimization level: " << opt_level_ << std::endl;
  os << "run until stage: " << stage_ << std::endl;
  os << std::endl;
  // show registed info
  os << "registed passes:" << std::endl;
  if (GetPasses().empty()) {
    os << "  <none>" << std::endl;
    return;
  }
  for (const auto &[name, info] : GetPasses()) {
    os << "  ";
    os << std::setw(20) << std::left << name;
    os << "min_opt_level = " << info.min_opt_level() << ", ";
    os << "pass_stage = " << info.stages() << std::endl;
  }
  os << std::endl;
  // show enabled passes
  int count = 0;
  os << "enabled passes:" << std::endl;
  for (const auto &[name, info] : GetPasses()) {
    if (opt_level_ >= info.min_opt_level() &&
        IsStageContain(stage_, info.stages())) {
      if (count % 5 == 0) os << "  ";
      os << std::setw(16) << std::left << name;
      if (count % 5 == 4) os << std::endl;
      ++count;
    }
  }
  if (!count) {
    os << "  <none>" << std::endl;
  }
  else if ((count - 1) % 5 != 4) {
    os << std::endl;
  }
}
