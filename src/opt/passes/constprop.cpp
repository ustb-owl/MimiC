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
      uint32_t result;
      switch (ssa.op()) {
        case BinarySSA::Operator::NotEq: {
          result = operand_[0] != operand_[1];
          break;
        }
        case BinarySSA::Operator::Add: {
          result = operand_[0] + operand_[1];
          break;
        }
        case BinarySSA::Operator::Sub: {
          result = operand_[0] - operand_[1];
          //          auto type = GenIntType();
          //          SSAPtr resultSSA = mod.GetInt(result, type);
          break;
        }
        case BinarySSA::Operator::Mul: {
          result = operand_[0] * operand_[1];
          break;
        }
        case BinarySSA::Operator::SDiv: {
          result =
              static_cast<int>(operand_[0]) / static_cast<int>(operand_[1]);
          break;
        }
        case BinarySSA::Operator::AShr: {
          result = static_cast<int>(operand_[0]) >>
                   static_cast<int>(operand_[1]);
          break;
        }
        case BinarySSA::Operator::Equal: {
          result = (operand_[0] == operand_[1]);
          break;
        }
        case BinarySSA::Operator::SLess: {
          result =
              static_cast<int>(operand_[0]) < static_cast<int>(operand_[1]);
          break;
        }
        case BinarySSA::Operator::SGreat: {
          result =
              static_cast<int>(operand_[0]) > static_cast<int>(operand_[1]);
          break;
        }
        case BinarySSA::Operator::SLessEq: {
          result = static_cast<int>(operand_[0]) <=
                   static_cast<int>(operand_[1]);
          break;
        }
        case BinarySSA::Operator::SGreatEq: {
          result = static_cast<int>(operand_[0]) >=
                   static_cast<int>(operand_[1]);
          break;
        }
        case BinarySSA::Operator::SRem: {
          result =
              static_cast<int>(operand_[0]) % static_cast<int>(operand_[1]);
          break;
        }

        /*无符号相关 */
        case BinarySSA::Operator::ULess: {
          result = operand_[0] < operand_[1];
          break;
        }
        case BinarySSA::Operator::UGreat: {
          result = operand_[0] > operand_[1];
          break;
        }
        case BinarySSA::Operator::ULessEq: {
          result = operand_[0] <= operand_[1];
          break;
        }
        case BinarySSA::Operator::UGreatEq: {
          result = operand_[0] >= operand_[1];
          break;
        }
        case BinarySSA::Operator::LShr: {
          result = operand_[0] >> operand_[1];
          break;
        }
        case BinarySSA::Operator::And: {
          result = operand_[0] & operand_[1];
          break;
        }
        case BinarySSA::Operator::Or: {
          result = operand_[0] | operand_[1];
          break;
        }
        case BinarySSA::Operator::Xor: {
          result = operand_[0] ^ operand_[1];
          break;
        }
        case BinarySSA::Operator::Shl: {
          result = operand_[0] << operand_[1];
          break;
        }
        case BinarySSA::Operator::UDiv: {
          result = operand_[0] / operand_[1];
          break;
        }
        case BinarySSA::Operator::URem: {
          result = operand_[0] % operand_[1];
          break;
        }
        default:
          operand_.clear();
          return;
      }
      finalSSA_ = GenIntResultSSA(result, mod);
      changed_ = needFold_ = true;
    }

    // Clear operand list after every round
    operand_.clear();
  }

  void RunOn(ConstIntSSA &ssa) override { operand_.push_back(ssa.value()); }

 private:
  bool needFold_{}, changed_{};
  std::vector<uint32_t> operand_;
  SSAPtr finalSSA_ = nullptr;

  static inline TypePtr GenIntType() {
    return MakePrimType(PrimType::Type::Int32, false);
  }
  static inline SSAPtr GenIntResultSSA(const uint32_t &result,
                                       Module &mod) {
    auto type = GenIntType();
    SSAPtr resultSSA = mod.GetInt(result, type);
    return resultSSA;
  }
};
}  // namespace

REGISTER_PASS(ConstPropagation, const_propagation, 0,
              PassStage::PreOpt | PassStage::Opt | PassStage::PostOpt);
