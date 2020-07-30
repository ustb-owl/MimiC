#include "opt/passman.h"

#include <iomanip>
#include <cassert>

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

std::list<PassInfo *> &PassManager::GetPasses() {
  static std::list<PassInfo *> passes;
  return passes;
}

std::list<PassInfo *> PassManager::GetPasses(PassStage stage) const {
  std::list<PassInfo *> passes;
  for (const auto &info : GetPasses()) {
    if ((info->stage() & stage) != PassStage::None &&
        opt_level_ >= info->min_opt_level()) {
      passes.push_front(info);
    }
  }
  return passes;
}

void PassManager::RunPasses(const std::list<PassInfo *> &passes) const {
  bool changed = true;
  // run until nothing changes
  while (changed) {
    changed = false;
    // traverse all passes
    for (const auto &info : passes) {
      const auto &pass = info->pass();
      // handle by pass type
      if (pass->IsModulePass()) {
        // run on global values
        if (pass->RunOnModule(*vars_)) changed = true;
        if (pass->RunOnModule(*funcs_)) changed = true;
      }
      else if (pass->IsFunctionPass()) {
        // traverse all functions
        for (const auto &func : *funcs_) {
          if (pass->RunOnFunction(func)) changed = true;
        }
      }
      else {
        assert(pass->IsBlockPass());
        // traverse all basic blocks
        for (const auto &func : *funcs_) {
          for (const auto &i : *func) {
            auto blk = std::static_pointer_cast<mid::BlockSSA>(i.value());
            if (pass->RunOnBlock(blk)) changed = true;
          }
        }
      }
    }
  }
}

void PassManager::RegisterPass(PassInfo *info) {
  GetPasses().push_back(info);
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
  for (const auto &i : GetPasses()) {
    os << "  ";
    os << std::setw(20) << std::left << i->name();
    os << "min_opt_level = " << i->min_opt_level() << ", ";
    os << "pass_stage = " << i->stage() << std::endl;
  }
  os << std::endl;
  // show enabled passes
  int count = 0;
  os << "enabled passes:" << std::endl;
  for (const auto &i : GetPasses()) {
    if (opt_level_ >= i->min_opt_level() &&
        IsStageContain(stage_, i->stage())) {
      if (count % 5 == 0) os << "  ";
      os << std::setw(16) << std::left << i->name();
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
