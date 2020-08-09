#include "opt/pass.h"
#include "opt/passman.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

// undefined value propagation
class UndefPropagationPass : public BlockPass {
 public:
  UndefPropagationPass() {}

  bool RunOnBlock(const BlockPtr &block) override {
    bool changed = false;
    auto mod = MakeModule(nullptr);
    // traverse all instructions
    auto &insts = block->insts();
    for (auto it = insts.begin(); it != insts.end();) {
      auto &inst = *it;
      if (inst->IsUndef()) {
        if (!changed) changed = true;
        // replace current value with undef
        auto context = mod.SetContext(inst->logger());
        inst->ReplaceBy(mod.GetUndef(inst->type()));
        // remove instruction
        it = insts.erase(it);
      }
      else {
        ++it;
      }
    }
    return changed;
  }
};

}  // namespace

// register current pass
REGISTER_PASS(UndefPropagationPass, undef_prop)
    .set_min_opt_level(1)
    .set_stages(PassStage::Opt);
