#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/passes/helper/cast.h"
#include "opt/passes/helper/const.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  phi simplify
  this pass will simplify all trivial phi nodes

  reference:  Simple and Efficient Construction of
              Static Single Assignment Form
*/
class PhiSimplifyPass : public BlockPass {
 public:
  PhiSimplifyPass() {}

  bool RunOnBlock(const BlockPtr &block) override {
    bool changed = false;
    // traverse all instructions
    auto &insts = block->insts();
    for (auto it = insts.begin(); it != insts.end();) {
      is_trivial_ = false;
      (*it)->RunPass(*this);
      if (is_trivial_) {
        // remove current instruction
        if (!changed) changed = true;
        it = insts.erase(it);
      }
      else {
        ++it;
      }
    }
    return changed;
  }

  void RunOn(PhiSSA &ssa) override {
    SSAPtr same;
    // scan all operands
    for (const auto &i : ssa) {
      auto op_ptr = SSACast<PhiOperandSSA>(i.value().get());
      const auto &op = op_ptr->value();
      // unique value or self-reference
      if (ConstantHelper::IsIdentical(op, same) || op.get() == &ssa) {
        continue;
      }
      // the phi node merges at least two values, not trivial
      if (same) return;
      // remember current operand
      same = op;
    }
    // check if is unreachable or in the start block
    if (!same) {
      auto mod = MakeModule(ssa.logger());
      same = mod.GetUndef(ssa.type());
      ssa.logger()->LogWarning("using uninitialized variable");
    }
    // reroute all uses of phi node to 'same'
    ssa.ReplaceBy(same);
    is_trivial_ = true;
  }

 private:
  bool is_trivial_;
};

}  // namespace

// register current pass
REGISTER_PASS(PhiSimplifyPass, phi_simp, 1, PassStage::Promote);
