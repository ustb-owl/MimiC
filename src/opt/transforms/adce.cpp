#include <vector>
#include <unordered_set>
#include <cassert>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/helper/cast.h"
#include "opt/helper/blkiter.h"
#include "opt/helper/inst.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  aggressive dead code elimination

  this pass will assume all instructions are dead,
  and try to prove some instructions are undead,
  then remove those dead instructions

  reference: Engeering a Compiler

  TODO: this is a naive implementation without calculating RDF
*/
class AggressiveDeadCodeElimPass : public FunctionPass {
 public:
  AggressiveDeadCodeElimPass() {}

  bool RunOnFunction(const FuncPtr &func) override {
    if (func->is_decl()) return false;
    Mark(func.get());
    return Sweep(func.get());
  }

  void CleanUp() override {
    assert(worklist_.empty());
    liveset_.clear();
  }

 private:
  void Mark(FunctionSSA *func);
  bool Sweep(FunctionSSA *func);

  std::vector<User *> worklist_;
  std::unordered_set<Value *> liveset_;
};

}  // namespace


// register current pass
REGISTER_PASS(AggressiveDeadCodeElimPass, adce)
    .set_min_opt_level(1)
    .set_stages(PassStage::Opt);


void AggressiveDeadCodeElimPass::Mark(FunctionSSA *func) {
  // traverse all blocks to find critical instructions
  auto entry = SSACast<BlockSSA>(func->entry().get());
  for (const auto &block : BFSTraverse(entry)) {
    for (const auto &i : block->insts()) {
      if (IsInstCritical(i)) {
        // mark as undead
        auto inst = InstCast(i.get());
        worklist_.push_back(inst);
        liveset_.insert(inst);
      }
    }
  }
  // mark more intructions
  while (!worklist_.empty()) {
    // get the last instruction
    auto inst = worklist_.back();
    worklist_.pop_back();
    // mark all of it's operand as undead
    for (const auto &opr : *inst) {
      const auto &val = opr.value();
      if (!val || liveset_.count(val.get())) continue;
      if (auto i = InstDynCast(val.get())) {
        worklist_.push_back(i);
        liveset_.insert(i);
      }
      else if (auto phi_opr = SSADynCast<PhiOperandSSA>(val.get())) {
        if (auto i = InstDynCast(phi_opr->value().get())) {
          worklist_.push_back(i);
          liveset_.insert(i);
        }
      }
    }
    // TODO: scan RDF blocks
  }
}

bool AggressiveDeadCodeElimPass::Sweep(FunctionSSA *func) {
  bool changed = false;
  // traverse all blocks
  for (const auto &block : *func) {
    auto block_ptr = SSACast<BlockSSA>(block.value().get());
    auto &insts = block_ptr->insts();
    for (auto it = insts.begin(); it != insts.end();) {
      // TODO: handle RDF blocks
      // check if instruction is unmarked
      if (!liveset_.count(it->get())) {
        // break circular reference
        (*it)->ReplaceBy(nullptr);
        it = insts.erase(it);
        changed = true;
      }
      else {
        ++it;
      }
    }
  }
  return changed;
}
