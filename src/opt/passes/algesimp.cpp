/* Author : dyf
   TODO: a / a => 1
   TODO: a and a => a && a or a => a
   TODO: a - a => 0
*/
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

class AlgebraicSimplification : public BlockPass {
 public:
  AlgebraicSimplification() {}

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

    // Identity simplification
    if (operand_.size() == 1) {
      if (left->IsConst() && (operand_[0] == 1)) {
        // 1 * a => a
        switch (ssa.op()) {
          case BinarySSA::Operator::Mul: {
            finalSSA_ = right;
            break;
          }

          default:
            return;
        }

        changed_ = needFold_ = true;
      }
      else if (left->IsConst() && (operand_[0] == 0)) {
        switch (ssa.op()) {
          // 0 + a => a
          case BinarySSA::Operator::Add: {
            finalSSA_ = right;
            break;
          }

          // 0 * a => 0 && 0 / a => 0
          case BinarySSA::Operator::Mul:
          case BinarySSA::Operator::SDiv:
          case BinarySSA::Operator::UDiv: {
            finalSSA_ = left;
            break;
          }
            // #endif
          default:
            return;
        }

        changed_ = needFold_ = true;
      }
      else if (right->IsConst() && (operand_[0] == 1)) {
        switch (ssa.op()) {
          // a * 1 => a && a / 1 => a
          case BinarySSA::Operator::Mul:
          case BinarySSA::Operator::UDiv:
          case BinarySSA::Operator::SDiv: {
            finalSSA_ = left;
            break;
          }

          default:
            return;
        }

        changed_ = needFold_ = true;
      }
      else if (right->IsConst() && (operand_[0] == 0)) {
        switch (ssa.op()) {
          // a + 0 => a && a - 0 => a
          // a << 0 => a && a >> 0 => a
          case BinarySSA::Operator::Add:
          case BinarySSA::Operator::Sub:
          case BinarySSA::Operator::Shl:
          case BinarySSA::Operator::LShr:
          case BinarySSA::Operator::AShr: {
            finalSSA_ = left;
            break;
          }

          // a * 0 => a
          case BinarySSA::Operator::Mul: {
            finalSSA_ = right;
            break;
          }

          // a / 0 => a
          case BinarySSA::Operator::SDiv:
          case BinarySSA::Operator::UDiv: {
            ssa.logger()->LogWarning(
                "ZeroDivisionError: integer division or modulo by zero");
          }

          default:
            return;
        }
      }
      else if (right->IsConst()) {
        if (ssa.op() == BinarySSA::Operator::SDiv) {
          if (Is2Power(operand_[0])) {  // 处理2的幂
            auto mod = MakeModule();
            auto type = MakePrimType(PrimType::Type::Int32, false);
            SSAPtr rhs = mod.GetInt(Log2(operand_[0]), type);
            auto newBinarySSA = mod.CreateShr(left, rhs);
            finalSSA_ = newBinarySSA;
            changed_ = needFold_ = true;
          }
          else {
            return;
          }
        }
      }
    }

    // Clear operand list after every round
    operand_.clear();
  }

  void RunOn(ConstIntSSA &ssa) override { operand_.push_back(ssa.value()); }

 private:
  bool needFold_, changed_;
  std::vector<int> operand_;
  SSAPtr finalSSA_ = nullptr;

  inline bool Is2Power(int32_t num) { return (((num) & (num - 1)) == 0); }
  inline int Log2(int32_t num) { return (int)log2(num); }
};
}  // namespace

REGISTER_PASS(AlgebraicSimplification, Algebraic_Simp, 0,
              PassStage::PreOpt | PassStage::Opt | PassStage::PostOpt );
