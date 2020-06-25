/* Author : dyf
   TODO: a / a => 1
   TODO: a and a => a && a or a => a
   TODO: a - a => 0
*/
#include <iostream>
#include <memory>
#include <vector>

#include "mid/pass.h"
#include "mid/passman.h"
#include "mid/ssa.h"

using namespace yulang::mid;

namespace {

class AlgebraicSimplification : public BlockPass {
 public:
  AlgebraicSimplification() {}

  bool RunOnBlock(const BlockPtr &block) override {
    changed_ = needFold_ = false;
    for (auto it : block->insts()) {
      it->RunPass(*this);
      if (needFold_) {
        std::cout << "Changed!" << std::endl;
        it->ReplaceBy(finalSSA_);
        finalSSA_ = nullptr;
        needFold_ = false;
      }
    }
    return changed_;
  }

  void RunOn(BinarySSA &ssa) override {
    std::cout << "Line_pos\t"
              << "Operator\t"
              << "left\t"
              << "right\n";
    std::cout << ssa.logger()->line_pos() << "\t";
    std::cout << (int)ssa.op() << "\t";

    SSAPtr left, right;
    left = ssa[0].value();
    right = ssa[1].value();

    left->RunPass(*this);
    right->RunPass(*this);

    std::cout << "Length of operand: " << operand_.size() << std::endl;
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

      changed_ = needFold_ = true;
    }

    for (auto i : operand_) {
      std::cout << i << "\t";
    }
    std::cout << std::endl;

    // Clear operand list after every round
    operand_.clear();
  }

  void RunOn(ConstIntSSA &ssa) override {
    std::cout << "Const: " << ssa.value() << std::endl;
    operand_.push_back(ssa.value());
  }

 private:
  bool needFold_, changed_;
  std::vector<int> operand_;
  std::uint32_t result_;
  SSAPtr finalSSA_ = nullptr;
};
}  // namespace

REGISTER_PASS(AlgebraicSimplification, Algebraic_Simp, 1, false);
