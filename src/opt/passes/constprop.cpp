#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include "mid/module.h"
#include "mid/ssa.h"
#include "opt/pass.h"
#include "opt/passman.h"

using namespace mimic::opt;
using namespace mimic::mid;
using namespace mimic::define;

namespace {

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#define LOG() \
  std::cout << '[' << __FILENAME__ << ":" << std::dec << __LINE__ << "] "

class ConstPropagation : public BlockPass {
 public:
  ConstPropagation() {}

  bool RunOnBlock(const BlockPtr &block) override {
    changed_ = needFold_ = false;
    for (auto &&it : block->insts()) {
      it->RunPass(*this);
      if (needFold_) {
        it->ReplaceBy(finalSSA_);
        it = finalSSA_;
        finalSSA_ = nullptr;
        needFold_ = false;
      }
    }
    return changed_;
  }

  void RunOn(BinarySSA &ssa) override {
    SSAPtr left, right;
    left = ssa[0].value();
    right = ssa[1].value();

    left->RunPass(*this);
    right->RunPass(*this);

    if (operand_.size() == 2) {
      auto mod = MakeModule();
      switch (ssa.op()) {
        case BinarySSA::Operator::Add: {
          int result = operand_[0] + operand_[1];
          finalSSA_ = GenIntResultSSA(result, mod);
          break;
        }
        case BinarySSA::Operator::Sub: {
          int result = operand_[0] - operand_[1];
          //          auto type = GenIntType();
          //          SSAPtr resultSSA = mod.GetInt(result, type);
          finalSSA_ = GenIntResultSSA(result, mod);
          break;
        }
        case BinarySSA::Operator::Mul: {
          int result = operand_[0] * operand_[1];
          finalSSA_ = GenIntResultSSA(result, mod);
          break;
        }
        case BinarySSA::Operator::AShr: {
          int result = operand_[0] >> operand_[1];
          finalSSA_ = GenIntResultSSA(result, mod);
          break;
        }
        case BinarySSA::Operator::Equal: {
          int result = (operand_[0] == operand_[1]);
          finalSSA_ = GenIntResultSSA(result, mod);
          break;
        }
        case BinarySSA::Operator::SLess: {
          int result = operand_[0] < operand_[1];
          finalSSA_ = GenIntResultSSA(result, mod);
          break;
        }
        case BinarySSA::Operator::SGreat: {
          int result = operand_[0] > operand_[1];
          finalSSA_ = GenIntResultSSA(result, mod);
          break;
        }
        case BinarySSA::Operator::SLessEq: {
          int result = operand_[0] <= operand_[1];
          finalSSA_ = GenIntResultSSA(result, mod);
          break;
        }
        case BinarySSA::Operator::SGreatEq: {
          int result = operand_[0] >= operand_[1];
          finalSSA_ = GenIntResultSSA(result, mod);
          break;
        }
        default:
          return;
      }
      changed_ = needFold_ = true;
    }

    // Clear operand list after every round
    operand_.clear();
  }

  void RunOn(ConstIntSSA &ssa) override { operand_.push_back(ssa.value()); }

 private:
  bool needFold_{}, changed_{};
  std::vector<int> operand_;
  SSAPtr finalSSA_ = nullptr;

  static inline bool Is2Power(int32_t num) {
    return (((num) & (num - 1)) == 0);
  }
  static inline int Log2(int32_t num) { return (int)log2(num); }
  static inline TypePtr GenIntType() {
    return MakePrimType(PrimType::Type::Int32, false);
  }
  static inline SSAPtr GenIntResultSSA(const int &result, Module &mod) {
    auto type = GenIntType();
    SSAPtr resultSSA = mod.GetInt(result, type);
    return resultSSA;
  }
};
}  // namespace

REGISTER_PASS(ConstPropagation, const_propagation, 0,
              PassStage::PreOpt | PassStage::Opt | PassStage::PostOpt);
