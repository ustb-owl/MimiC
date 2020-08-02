#include "opt/pass.h"
#include "opt/passman.h"
#include "mid/module.h"

#include <optional>
#include <cmath>

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  replace 'mul %x, 2^k' with 'shl %x, k'
*/
class IRDemoPass : public BlockPass {
 public:
  IRDemoPass() {}

  bool RunOnBlock(const BlockPtr &block) override {
    bool changed = false;
    // traverse all instructions
    for (auto &&i : block->insts()) {
      // check if can be replaced
      opr_ = nullptr;
      i->RunPass(*this);
      if (opr_) {
        // create bit shifting
        auto mod = MakeModule(i->logger());
        auto shl = mod.CreateShl(opr_, mod.GetInt(*k_, i->type()));
        // replace with new IR
        i->ReplaceBy(shl);
        i = shl;
        // mark as changed
        if (!changed) changed = true;
      }
    }
    return changed;
  }

  void RunOn(BinarySSA &ssa) override {
    // must be a multiplication
    if (ssa.op() != BinarySSA::Operator::Mul) return;
    // check operands
    if (ssa.lhs()->IsConst() && CheckOperand(ssa.rhs(), ssa.lhs())) {
      opr_ = ssa.rhs();
    }
    else if (ssa.rhs()->IsConst() && CheckOperand(ssa.lhs(), ssa.rhs())) {
      opr_ = ssa.lhs();
    }
  }

  void RunOn(ConstIntSSA &ssa) override {
    k_ = ssa.value();
  }

 private:
  bool CheckOperand(const SSAPtr &opr, const SSAPtr &num) {
    // try to fetch the constant operand
    k_ = {};
    num->RunPass(*this);
    // check if match form '2^k'
    if (k_ && !(*k_ & (*k_ - 1))) {
      k_ = std::log2(*k_);
      return true;
    }
    return false;
  }

  SSAPtr opr_;
  std::optional<std::uint32_t> k_;
};

}  // namespace

// register current pass
//REGISTER_PASS(IRDemoPass, ir_demo, 1, PassStage::Opt);
