#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>
#include <cstddef>
#include <cassert>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/helper/cast.h"
#include "opt/helper/ircopier.h"
#include "opt/analysis/loopinfo.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  parameters used by function inlining pass
*/
// threshold of callee's instruction count
constexpr std::size_t kCalleeInstThreshold = 1 << 7;
// threshold of caller's instruction count
constexpr std::size_t kCallerInstThreshold = 1 << 9;
// threshold of recursive function's instruction count
constexpr std::size_t kRecFuncInstThreshold = 1 << 7;
// threshold of recursive function's inline count
constexpr std::size_t kRecFuncInlineCountThreshold = 3;
// threshold of in-loop function's basic block count
constexpr std::size_t kInLoopFuncBlockCountThreshold = 3;
// threshold of in-loop function's instruction count
constexpr std::size_t kInLoopFuncInstCountThreshold = 1 << 7;


// helper class for block splitting
class BlockSplitterHelperPass : public HelperPass {
 public:
  // divide the block into two from pos, instruction on pos will be removed
  // NOTE: this method will not add terminator to current block
  void SplitBlock(BlockSSA *block, SSAPtrList::iterator pos) {
    cur_ = block;
    auto &insts = block->insts();
    assert(pos != insts.end() && *pos != insts.back());
    // create new block
    auto mod = MakeModule(block->logger());
    blk_ = mod.CreateBlock(block->parent(), block->name() + ".split");
    // copy instructions after pos to new block
    blk_->insts().insert(blk_->insts().end(), ++pos, insts.end());
    // handle terminator, update successor's predecessor
    insts.back()->RunPass(*this);
    // remove instructions from pos to then end
    insts.erase(--pos, insts.end());
  }

  void RunOn(BranchSSA &ssa) override {
    ReplacePred(SSACast<BlockSSA>(ssa.true_block().get()));
    ReplacePred(SSACast<BlockSSA>(ssa.false_block().get()));
  }

  void RunOn(JumpSSA &ssa) override {
    ReplacePred(SSACast<BlockSSA>(ssa.target().get()));
  }

  // getters
  const BlockPtr &new_block() const { return blk_; }

 private:
  void ReplacePred(BlockSSA *block) {
    // update predecessor reference of block
    for (auto &&i : *block) {
      if (i.value().get() == cur_) {
        i.set_value(blk_);
        break;
      }
    }
    // update phi node in block
    for (const auto &i : block->insts()) {
      auto phi = SSADynCast<PhiSSA>(i.get());
      if (!phi) continue;
      for (const auto &opr : *phi) {
        auto opr_ptr = SSACast<PhiOperandSSA>(opr.value().get());
        if (opr_ptr->block().get() == cur_) {
          opr_ptr->set_block(blk_);
        }
      }
    }
  }

  // current block
  BlockSSA *cur_;
  // newly created block
  BlockPtr blk_;
};

// helper pass for copying all blocks of a function
class InlineHelperPass : public IRCopier {
 public:
  using IRCopier::RunOn;

  void CopyTarget(const FuncPtr &cur, CallSSA *call) {
    cur_func_ = cur;
    cur_entry_ = SSACast<BlockSSA>(cur->entry().get());
    auto target = SSACast<FunctionSSA>(call->callee().get());
    // remember all arguments
    for (std::size_t i = 1; i < call->size(); ++i) {
      AddCopiedValue(target->args()[i - 1].get(), (*call)[i].value());
    }
    // traverse all blocks
    auto target_entry = SSACast<BlockSSA>(target->entry().get());
    target_entry->RunPass(*this);
  }

  void RunOn(BlockSSA &ssa) override {
    // create a new block
    auto block = CopyFromValue(ssa, cur_func_, ssa.name());
    block->Resize(ssa.size());
    CopyOperand(block, ssa);
    cur_func_->AddValue(block);
    if (!entry_) entry_ = block;
    // copy all instructions
    for (const auto &i : ssa.insts()) {
      auto inst = GetCopy(i);
      if (IsSSA<AllocaSSA>(inst)) {
        // alloca instruction, insert to current function's entry
        cur_entry_->insts().push_front(inst);
      }
      else if (auto ret = SSADynCast<ReturnSSA>(inst.get())) {
        assert(!exit_ && !ret_val_);
        // return instruction, mark current block as exit block
        exit_ = block;
        // get return value
        ret_val_ = ret->value();
      }
      else {
        // insert to block
        block->insts().push_back(inst);
      }
    }
  }

  // getters
  const BlockPtr &entry() const { return entry_; }
  const BlockPtr &exit() const { return exit_; }
  const SSAPtr &ret_val() const { return ret_val_; }

 private:
  // pointer to copied entry block & exit block
  BlockPtr entry_, exit_;
  // return value of current function
  SSAPtr ret_val_;
  // current function
  FuncPtr cur_func_;
  // entry block of current function
  BlockSSA *cur_entry_;
};


/*
  perform simple function inlining
  this pass will:
  1.  record necessary information of all functions
      (such as instruction count)
  2.  perform function inlining if conditions are met
*/
class FunctionInliningPass : public FunctionPass {
 public:
  FunctionInliningPass() {}

  bool RunOnFunction(const FuncPtr &func) override {
    if (func->is_decl()) return false;
    // update information of in-loop calls
    UpdateInLoopInfo(func.get());
    // traverse all instructions
    bool changed = true;
    while (changed) {
      changed = false;
      for (const auto &i : *func) {
        auto block = SSACast<BlockSSA>(i.value());
        if (ScanBlock(func, block)) {
          changed = true;
          break;
        }
      }
    }
    return changed;
  }

  void CleanUp() override {
    func_info_.clear();
  }

 private:
  struct FuncInfo {
    // basic block count
    std::size_t block_count;
    // instruction count
    std::size_t inst_count;
    // count of contained function calls (including recursive call)
    std::size_t func_call_count;
    // function is recursive
    bool is_recursive;
  };

  void UpdateFuncInfo(FunctionSSA *func);
  const FuncInfo &GetFuncInfo(FunctionSSA *func);
  void UpdateInLoopInfo(FunctionSSA *func);
  bool IsInLoop(FunctionSSA *cur, CallSSA *call);
  bool CanInline(FunctionSSA *cur, CallSSA *call);
  void IncreaseInlineCount(CallSSA *call);
  bool ScanBlock(const FuncPtr &func, const BlockPtr &block);

  // function information
  std::unordered_map<FunctionSSA *, FuncInfo> func_info_;
  // inline counter
  // NOTE: the data will not be cleared between pass calls
  std::unordered_map<FunctionSSA *, std::size_t> inline_count_;
  // all in-loop function calls in function
  // NOTE: the data will not be cleared between pass calls
  std::unordered_map<FunctionSSA *, std::unordered_set<Value *>>
      in_loop_calls_;
};

}  // namespace

// register current pass
REGISTER_PASS(FunctionInliningPass, inliner)
    .set_min_opt_level(2)
    .set_stages(PassStage::Opt)
    .Requires("loop_info")
    .Requires("naive_unroll")
    .Requires("loop_conv")
    .Invalidates("dom_info");


// regather & update information of the specific function
void FunctionInliningPass::UpdateFuncInfo(FunctionSSA *func) {
  auto &info = func_info_[func];
  for (const auto &block : *func) {
    auto block_ptr = SSACast<BlockSSA>(block.value().get());
    ++info.block_count;
    for (const auto &inst : block_ptr->insts()) {
      ++info.inst_count;
      if (auto call = SSADynCast<CallSSA>(inst)) {
        ++info.func_call_count;
        // NOTE:  mimic does not supported function pointer yet
        //        so this assertion can be established
        assert(IsSSA<FunctionSSA>(call->callee()));
        if (call->callee().get() == func) info.is_recursive = true;
      }
    }
  }
}

// get function information from map
const FunctionInliningPass::FuncInfo &FunctionInliningPass::GetFuncInfo(
    FunctionSSA *func) {
  auto it = func_info_.find(func);
  if (it != func_info_.end()) {
    return it->second;
  }
  else {
    UpdateFuncInfo(func);
    return func_info_[func];
  }
}

// get all in-loop function calls of the specific function
// initialize only once for each function
void FunctionInliningPass::UpdateInLoopInfo(FunctionSSA *func) {
  // create new entry
  auto [it, succ] = in_loop_calls_.insert({func, {}});
  if (!succ) return;
  // detect all loops in function
  const auto &li = PassManager::GetPass<LoopInfoPass>("loop_info");
  const auto &loops = li.GetLoopInfo(func);
  // find out all call instructions in loop
  std::unordered_set<BlockSSA *> visited;
  for (const auto &loop : loops) {
    for (const auto &block : loop.body) {
      if (!visited.insert(block).second) continue;
      for (const auto &inst : block->insts()) {
        if (IsSSA<CallSSA>(inst)) it->second.insert(inst.get());
      }
    }
  }
}

// check if the specific call instruction is in a loop
bool FunctionInliningPass::IsInLoop(FunctionSSA *cur, CallSSA *call) {
  auto it = in_loop_calls_.find(cur);
  assert(it != in_loop_calls_.end());
  return it->second.count(call);
}

// check if target function of call instruction
// can be inlined into current function
bool FunctionInliningPass::CanInline(FunctionSSA *cur, CallSSA *call) {
  auto target = SSACast<FunctionSSA>(call->callee().get());
  if (target->is_decl()) return false;
  const auto &cur_info = GetFuncInfo(cur);
  const auto &target_info = GetFuncInfo(target);
  // do not inline if caller is too large
  if (cur_info.inst_count > kCallerInstThreshold) return false;
  // do not inline if is calling another recursive function
  // do not inline another function if current is recursive
  if (cur != target &&
      (cur_info.is_recursive || target_info.is_recursive)) {
    return false;
  }
  // check for in-loop function calls
  if (IsInLoop(cur, call)) {
    return !target_info.is_recursive &&
           target_info.block_count <= kInLoopFuncBlockCountThreshold &&
           target_info.inst_count <= kInLoopFuncInstCountThreshold;
  }
  // inline functions that has only been called once anyway
  if (cur != target && target->uses().size() == 1) return true;
  // check if satisfied the instruction count threshold
  if (target_info.is_recursive) {
    // recursive unrolling
    if (inline_count_[target] >= kRecFuncInlineCountThreshold ||
        target_info.inst_count > kRecFuncInstThreshold) {
      return false;
    }
  }
  else if (target_info.inst_count > kCalleeInstThreshold) {
    return false;
  }
  return true;
}

// increase inline counter of call instruction's target function
void FunctionInliningPass::IncreaseInlineCount(CallSSA *call) {
  auto target = SSACast<FunctionSSA>(call->callee().get());
  ++inline_count_[target];
}

// scan and perform function inlining, returns true if inlined
bool FunctionInliningPass::ScanBlock(const FuncPtr &func,
                                     const BlockPtr &block) {
  auto &insts = block->insts();
  for (auto it = insts.begin(); it != insts.end(); ++it) {
    auto call = SSADynCast<CallSSA>(*it);
    if (call && CanInline(func.get(), call.get())) {
      // copy target function
      InlineHelperPass inliner;
      inliner.CopyTarget(func, call.get());
      // split current block
      BlockSplitterHelperPass splitter;
      splitter.SplitBlock(block.get(), it);
      // create new terminators
      auto mod = MakeModule(block->logger(), block);
      mod.CreateJump(inliner.entry());
      mod.SetInsertPoint(inliner.exit());
      mod.CreateJump(splitter.new_block());
      // update return value
      if (!call->uses().empty()) call->ReplaceBy(inliner.ret_val());
      // update function info of current function
      UpdateFuncInfo(func.get());
      IncreaseInlineCount(call.get());
      return true;
    }
  }
  return false;
}
