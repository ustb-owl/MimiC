#include "back/asm/arch/archinfo.h"

#include <iomanip>
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

void ArchManager::ShowArchs(std::ostream &os) {
  os << "supported target architectures:" << std::endl;
  int count = 0;
  for (const auto &[name, _] : GetArchs()) {
    if (count % 5 == 0) os << "  ";
    os << std::setw(16) << std::left << name;
    if (count % 5 == 4) os << std::endl;
    ++count;
  }
  assert(count);
  if ((count - 1) % 5 != 4) os << std::endl;
}
