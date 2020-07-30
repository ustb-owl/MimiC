#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/passes/helper/cast.h"
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
    std::uint32_t same_val;
    // scan all operands
    for (const auto &i : ssa) {
      auto op_ptr = SSACast<PhiOperandSSA>(i.value().get());
      const auto &op = op_ptr->value();
      // unique value or self-reference
      if (op == same || op.get() == &ssa) continue;
      // check if is constant
      if (op->IsConst() && same && same->IsConst()) {
        op->RunPass(*this);
        // same constant, continue
        if (val_ == same_val) continue;
      }
      // the phi node merges at least two values, not trivial
      if (same) return;
      // remember current operand
      same = op;
      if (same->IsConst()) {
        assert(same->type()->IsInteger());
        same->RunPass(*this);
        same_val = val_;
      }
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

  void RunOn(ConstIntSSA &ssa) override { val_ = ssa.value(); }
  void RunOn(ConstZeroSSA &ssa) override { val_ = 0; }

 private:
  bool is_trivial_;
  std::uint32_t val_;
};

}  // namespace

// register current pass
REGISTER_PASS(PhiSimplifyPass, phi_simp, 1, PassStage::Promote);
