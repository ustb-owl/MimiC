#include "back/asm/arch/archinfo.h"

#include <cassert>

using namespace mimic::back::asmgen;

std::unordered_map<std::string_view, ArchEntry *> &ArchManager::GetArchs() {
  static std::unordered_map<std::string_view, ArchEntry *> archs;
  return archs;
}

void ArchManager::RegisterArch(std::string_view name, ArchEntry *arch) {
  auto succ = GetArchs().insert({name, arch}).second;
  assert(succ);
  static_cast<void>(succ);
}

ArchInfoPtr ArchManager::GetArch(std::string_view name) {
  auto it = GetArchs().find(name);
  if (it != GetArchs().end()) {
    return it->second->arch_info();
  }
  else {
    return nullptr;
  }
}
