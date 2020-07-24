#ifndef MIMIC_BACK_ASM_MIR_LABEL_H_
#define MIMIC_BACK_ASM_MIR_LABEL_H_

#include <string>
#include <unordered_map>
#include <cstdint>

#include "back/asm/mir/mir.h"

namespace mimic::back::asmgen {

// label reference
class LabelOperand : public OperandBase {
 public:
  LabelOperand(const std::string &label) : label_(label) {}

  bool IsReg() const override { return false; }
  bool IsVirtual() const override { return false; }
  bool IsImm() const override { return false; }
  bool IsLabel() const override { return true; }
  bool IsSlot() const override { return false; }

  void Dump(std::ostream &os) const override { os << label_; }

  // getters
  const std::string &label() const { return label_; }

 private:
  std::string label_;
};

// label factory
class LabelFactory {
 public:
  LabelFactory() : next_id_(0) {}

  // get a new named label
  OprPtr GetLabel(const std::string &label) {
    auto it = named_labels_.find(label);
    if (it != named_labels_.end()) {
      return it->second;
    }
    else {
      auto opr = std::make_shared<LabelOperand>(label);
      named_labels_.insert({label, opr});
      return opr;
    }
  }

  // get a new anonymous label
  OprPtr GetLabel() {
    return std::make_shared<LabelOperand>(".L" +
                                          std::to_string(next_id_++));
  }

 private:
  std::unordered_map<std::string, OprPtr> named_labels_;
  std::uint32_t next_id_;
};

}  // namespace mimic::back::asmgen

#endif  // MIMIC_BACK_ASM_MIR_LABEL_H_
