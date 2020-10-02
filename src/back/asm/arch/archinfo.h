#ifndef MIMIC_BACK_ASM_ARCH_ARCHINFO_H_
#define MIMIC_BACK_ASM_ARCH_ARCHINFO_H_

#include <memory>
#include <string_view>
#include <ostream>
#include <unordered_map>
#include <cstddef>

#include "back/asm/arch/instgen.h"
#include "back/asm/mir/pass.h"

namespace mimic::back::asmgen {

// architecture information
// base class of all architecture information
class ArchInfoBase {
 public:
  virtual ~ArchInfoBase() = default;

  // return pointer size of current architecture
  virtual std::size_t GetPtrSize() const = 0;
  // return instruction generator of current architecture
  virtual InstGenBase &GetInstGen() = 0;
  // return a list of required passes
  virtual PassPtrList GetPassList(std::size_t opt_level) = 0;
};

// pointer to architecture information
using ArchInfoPtr = std::shared_ptr<ArchInfoBase>;

// entry of registered architecture
class ArchEntry {
 public:
  ArchEntry(ArchInfoPtr arch_info) : arch_info_(std::move(arch_info)) {}
  virtual ~ArchEntry() = default;

  // getters
  const ArchInfoPtr &arch_info() const { return arch_info_; }

 private:
  ArchInfoPtr arch_info_;
};

// architecture information manager
class ArchManager {
 public:
  // register an architecture
  static void RegisterArch(std::string_view name, ArchEntry *arch);
  // get architecture information by name
  // returns 'nullptr' if architecture not found
  static ArchInfoPtr GetArch(std::string_view name);
  // show all registered architectures
  static void ShowArchs(std::ostream &os);

 private:
  // get architecture info list
  static std::unordered_map<std::string_view, ArchEntry *> &GetArchs();
};

// helper class for registering an architecture
template <typename T>
class RegisterArch : public ArchEntry {
 public:
  RegisterArch(std::string_view name) : ArchEntry(std::make_shared<T>()) {
    ArchManager::RegisterArch(name, this);
  }
};

// macro for architecture registration
#define REGISTER_ARCH(cls, name) static RegisterArch<cls> arch_##name(#name)

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_ARCH_ARCHINFO_H_
