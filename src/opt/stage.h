#ifndef MIMIC_OPT_STAGE_H_
#define MIMIC_OPT_STAGE_H_

#include <type_traits>
#include <string_view>
#include <cmath>

namespace mimic::opt {

// stage of pass running
// represented as bit flags
enum class PassStage : unsigned {
  None    = 0,
  PreOpt  = 1 << 0,
  Promote = 1 << 1,
  Opt     = 1 << 2,
  Demote  = 1 << 3,
  PostOpt = 1 << 4,
};

// the first/last pass stage
constexpr PassStage kFirstPassStage = PassStage::PreOpt;
constexpr PassStage kLastPassStage = PassStage::PostOpt;

// count of stages
constexpr std::size_t kPassStageCount = 5;

// set specific stage
inline PassStage operator|(PassStage lhs, PassStage rhs) {
  using T = std::underlying_type_t<PassStage>;
  return static_cast<PassStage>(static_cast<T>(lhs) | static_cast<T>(rhs));
}
inline PassStage &operator|=(PassStage &lhs, PassStage rhs) {
  return lhs = lhs | rhs;
}

// check if specific stage is set
inline PassStage operator&(PassStage lhs, PassStage rhs) {
  using T = std::underlying_type_t<PassStage>;
  return static_cast<PassStage>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

// switch to next pass stage
inline PassStage &operator++(PassStage &cur) {
  using T = std::underlying_type_t<PassStage>;
  cur = static_cast<PassStage>(static_cast<T>(cur) << 1);
  return cur;
}

// check if stage range [first, last] contains any stage in stage set
inline bool IsStageContain(PassStage last, PassStage stages) {
  using T = std::underlying_type_t<PassStage>;
  auto l = static_cast<T>(last), s = static_cast<T>(stages);
  return (l | (l - 1)) & s;
}

// get pass stage by name
inline PassStage GetStageByName(std::string_view name) {
  if (name == "PreOpt" || name == "preopt") return PassStage::PreOpt;
  if (name == "Promote" || name == "promote") return PassStage::Promote;
  if (name == "Opt" || name == "opt") return PassStage::Opt;
  if (name == "Demote" || name == "demote") return PassStage::Demote;
  if (name == "PostOpt" || name == "postopt") return PassStage::PostOpt;
  return PassStage::None;
}

}  // namespace mimic::opt

#endif  // MIMIC_OPT_STAGE_H_
