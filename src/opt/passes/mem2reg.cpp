#include <unordered_set>
#include <queue>
#include <unordered_map>
#include <cassert>

#include "opt/pass.h"
#include "opt/passman.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

// get all promotable alloca instructions
class GetPromAllocaHelperPass : public HelperPass {
 public:
  // scan for all alloca instructions
  // return true if there are promotable allocas
  bool ScanAlloca(BlockSSA *entry) {
    prom_allocas_.clear();
    for (const auto &i : entry->insts()) i->RunPass(*this);
    return !prom_allocas_.empty();
  }

  // check if specific value is a promotable alloca instruction
  bool IsPromotable(const SSAPtr &value) const {
    return prom_allocas_.find(value.get()) != prom_allocas_.end();
  }

  void RunOn(AllocaSSA &ssa) override {
    is_prom_ = true;
    // traverse on all users
    for (const auto &i : ssa.uses()) {
      i->user()->RunPass(*this);
      if (!is_prom_) return;
    }
    // add to set
    prom_allocas_.insert(&ssa);
  }

  void RunOn(AccessSSA &ssa) override {
    is_prom_ = false;
  }

 private:
  bool is_prom_;
  std::unordered_set<Value *> prom_allocas_;
};

// check if value is a phi/alloca
class PhiCheckerHelperPass : public HelperPass {
 public:
  bool IsPhi(Value *value) {
    is_phi_ = false;
    value->RunPass(*this);
    return is_phi_;
  }

  void RunOn(PhiSSA &ssa) override { is_phi_ = true; }

 private:
  bool is_phi_;
};

/*
  promote alloca instructions to SSA form (if possible)

  reference:  Simple and Efficient Construction of
              Static Single Assignment Form
  * made some simplification
*/
class MemToRegPass : public FunctionPass {
 public:
  MemToRegPass() {}

  bool RunOnFunction(const UserPtr &func) override {
    // check if need to run
    auto entry = static_cast<BlockSSA *>((*func)[0].value().get());
    if (!prom_helper_.ScanAlloca(entry)) return false;
    // traverse all blocks
    for (const auto &i : *func) {
      i.value()->RunPass(*this);
    }
    // handle created phi nodes
    while (!created_phis_.empty()) {
      const auto &[phi, block, alloca] = created_phis_.front();
      // check if current phi node is used by other user
      if (!phi->uses().empty()) {
        // add operands for current phi node
        AddPhiOperands(phi, block, alloca);
        // insert to block
        block->insts().push_front(phi);
      }
      created_phis_.pop();
    }
    // remove all promotable allocas
    auto &entry_insts = entry->insts();
    for (auto it = entry_insts.begin(); it != entry_insts.end();) {
      if (prom_helper_.IsPromotable(*it)) {
        // TODO:  '(*it)->uses().empty()' is not always be true
        //        memory leak?
        it = entry_insts.erase(it);
      }
      else {
        ++it;
      }
    }
    // release local definitions
    cur_defs_.clear();
    return true;
  }

  void RunOn(BlockSSA &ssa) override {
    cur_block_ = &ssa;
    // traverse instructions
    auto &insts = ssa.insts();
    for (auto it = insts.begin(); it != insts.end();) {
      remove_flag_ = false;
      (*it)->RunPass(*this);
      // check if need to remove
      if (remove_flag_) {
        it = insts.erase(it);
      }
      else {
        ++it;
      }
    }
  }

  void RunOn(LoadSSA &ssa) override {
    const auto &alloca = ssa[0].value();
    // check if is promotable
    if (!prom_helper_.IsPromotable(alloca)) return;
    // read value from local definitions
    auto val = ReadVariable(cur_block_, alloca);
    ssa.ReplaceBy(val);
    // mark as removed
    remove_flag_ = true;
  }

  void RunOn(StoreSSA &ssa) override {
    const auto &alloca = ssa[1].value();
    // check if is promotable
    if (!prom_helper_.IsPromotable(alloca)) return;
    // write value
    WriteVariable(cur_block_, alloca, ssa[0].value());
    // mark as removed
    remove_flag_ = true;
  }

 private:
  struct PhiInfo {
    // phi node
    UserPtr phi;
    // block where phi node located
    BlockSSA *block;
    // alloca where value of phi node comes from
    SSAPtr alloca;
  };

  UserPtr CreatePhi(const SSAPtr &alloca, BlockSSA *block);

  void WriteVariable(BlockSSA *block, const SSAPtr &alloca,
                     const SSAPtr &val);
  SSAPtr ReadVariable(BlockSSA *block, const SSAPtr &alloca);
  SSAPtr ReadVariableRecursive(BlockSSA *block, const SSAPtr &alloca);
  void AddPhiOperands(const UserPtr &phi, BlockSSA *block,
                      const SSAPtr &alloca);
  void TryRemoveTrivialPhi(const UserPtr &phi);

  // helper pass
  GetPromAllocaHelperPass prom_helper_;
  // current block & remove flag
  BlockSSA *cur_block_;
  bool remove_flag_;
  // local variable definitions
  std::unordered_map<BlockSSA *, std::unordered_map<Value *, SSAPtr>>
      cur_defs_;
  // all created phi nodes
  std::queue<PhiInfo> created_phis_;
};

}  // namespace

// register current pass
REGISTER_PASS(MemToRegPass, mem2reg, 1, PassStage::Promote);


// create an empty phi node, use alloca's type & logger
UserPtr MemToRegPass::CreatePhi(const SSAPtr &alloca, BlockSSA *block) {
  // create phi node
  auto phi = std::make_shared<PhiSSA>(SSAPtrList());
  phi->set_type(alloca->type()->GetDerefedType());
  phi->set_logger(alloca->logger());
  // remember it
  created_phis_.push({phi, block, alloca});
  return phi;
}

// write value to local definitions
void MemToRegPass::WriteVariable(BlockSSA *block, const SSAPtr &alloca,
                                 const SSAPtr &val) {
  cur_defs_[block][alloca.get()] = val;
}

// read value from definitions
SSAPtr MemToRegPass::ReadVariable(BlockSSA *block, const SSAPtr &alloca) {
  const auto &defs = cur_defs_[block];
  auto it = defs.find(alloca.get());
  if (it != defs.end()) {
    // local value
    return it->second;
  }
  else {
    // global value
    return ReadVariableRecursive(block, alloca);
  }
}

// read value from global definitions, create phi node when necessary
SSAPtr MemToRegPass::ReadVariableRecursive(BlockSSA *block,
                                           const SSAPtr &alloca) {
  SSAPtr val;
  if (block->size() == 1) {
    // block only has one predecessor, no phi needed
    auto pred = static_cast<BlockSSA *>((*block)[0].value().get());
    val = ReadVariable(pred, alloca);
  }
  else {
    // create an empty phi node
    val = CreatePhi(alloca, block);
  }
  WriteVariable(block, alloca, val);
  return val;
}

// add operands to specific phi node by looking up definition
// param block: block where the phi node is located
void MemToRegPass::AddPhiOperands(const UserPtr &phi, BlockSSA *block,
                                  const SSAPtr &alloca) {
  phi->Reserve(block->size());
  // determine operands from predecessors
  for (const auto &i : *block) {
    // read value
    auto pred = std::static_pointer_cast<BlockSSA>(i.value());
    auto val = ReadVariable(pred.get(), alloca);
    assert(val->type()->IsIdentical(phi->type()));
    // create phi operand
    auto mod = MakeModule();
    phi->AddValue(mod.CreatePhiOperand(val, pred));
  }
  TryRemoveTrivialPhi(phi);
}

// detect and recursively remove a trivial phi node
void MemToRegPass::TryRemoveTrivialPhi(const UserPtr &phi) {
  SSAPtr same;
  for (const auto &i : *phi) {
    auto op_ptr = static_cast<PhiOperandSSA *>(i.value().get());
    const auto &op = (*op_ptr)[0].value();
    // unique value or self-reference
    if (op == same || op == phi) continue;
    // the phi node merges at least two values, not trivial
    if (same) return;
    // remember current operand
    same = op;
  }
  // check if is unreachable or in the start block
  if (!same) {
    auto mod = MakeModule();
    same = mod.GetUndef(phi->type());
    phi->logger()->LogWarning("using uninitialized variable");
  }
  // remember all uses
  auto uses = phi->uses();
  // reroute all uses of phi node to 'same'
  phi->ReplaceBy(same);
  // try to recursively remove all phi users (except current phi)
  for (const auto &i : uses) {
    if (i->user() == phi.get()) continue;
    // check if user is a phi node
    PhiCheckerHelperPass checker;
    if (checker.IsPhi(i->user())) {
      // try to get pointer to current user (tricky method)
      const auto &uses = i->user()->uses();
      assert(!uses.empty());
      auto user = std::static_pointer_cast<User>(uses.front()->value());
      // handle current user
      TryRemoveTrivialPhi(user);
    }
  }
}
