#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>
#include <cstddef>
#include <cassert>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/passes/helper/cast.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  arguments used by function inlining pass
*/
// threshold of callee's instruction count
constexpr std::size_t kCalleeInstThreshold = 1 << 8;
// threshold of caller's instruction count
constexpr std::size_t kCallerInstThreshold = 1 << 14;
// threshold of recursive function's instruction count
constexpr std::size_t kRecFuncInstThreshold = 1 << 8;
// threshold of recursive function's inline count
constexpr std::size_t kRecFuncInlineCountThreshold = 3;


// pointer to function
using FuncPtr = std::shared_ptr<FunctionSSA>;

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
class InlineHelperPass : public HelperPass {
 public:
  void CopyTarget(const FuncPtr &cur, CallSSA *call) {
    cur_func_ = cur;
    cur_entry_ = SSACast<BlockSSA>(cur->entry().get());
    // remember all arguments
    for (std::size_t i = 1; i < call->size(); ++i) {
      args_.push_back((*call)[i].value());
    }
    // traverse all blocks
    auto target = SSACast<FunctionSSA>(call->callee().get());
    auto target_entry = SSACast<BlockSSA>(target->entry().get());
    target_entry->RunPass(*this);
  }

  void RunOn(BlockSSA &ssa) override {
    // create a new block
    auto mod = MakeModule(ssa.logger());
    auto block = CopyFromValue(ssa, cur_func_, ssa.name());
    block->Resize(ssa.size());
    CopyOperand(block, ssa);
    cur_func_->AddValue(block);
    // remember block
    blocks_.push_back(block);
    if (!entry_) entry_ = block;
    // copy all instructions
    for (const auto &i : ssa.insts()) {
      auto inst = GetCopy(i);
      if (IsSSA<AllocaSSA>(inst)) {
        // alloca instruction, insert to current function's entry
        cur_entry_->insts().push_front(inst);
      }
      else if (auto ret = SSADynCast<ReturnSSA>(inst.get())) {
        // return instruction, mark current block as exit block
        exit_ = block;
        // get return value
        assert(!ret_val_);
        ret_val_ = ret->value();
      }
      else {
        // insert to block
        block->insts().push_back(inst);
      }
    }
  }

  void RunOn(LoadSSA &ssa) override {
    auto load = CopyFromValue(ssa, nullptr);
    CopyOperand(load, ssa);
  }

  void RunOn(StoreSSA &ssa) override {
    auto store = CopyFromValue(ssa, nullptr, nullptr);
    CopyOperand(store, ssa);
  }

  void RunOn(AccessSSA &ssa) override {
    auto acc = CopyFromValue(ssa, ssa.acc_type(), nullptr, nullptr);
    CopyOperand(acc, ssa);
  }

  void RunOn(BinarySSA &ssa) override {
    auto bin = CopyFromValue(ssa, ssa.op(), nullptr, nullptr);
    CopyOperand(bin, ssa);
  }

  void RunOn(UnarySSA &ssa) override {
    auto una = CopyFromValue(ssa, ssa.op(), nullptr);
    CopyOperand(una, ssa);
  }

  void RunOn(CastSSA &ssa) override {
    auto cast = CopyFromValue(ssa, nullptr);
    CopyOperand(cast, ssa);
  }

  void RunOn(CallSSA &ssa) override {
    auto call = CopyFromValue(ssa, nullptr, SSAPtrList(ssa.size() - 1));
    CopyOperand(call, ssa);
  }

  void RunOn(BranchSSA &ssa) override {
    auto branch = CopyFromValue(ssa, nullptr, nullptr, nullptr);
    CopyOperand(branch, ssa);
  }

  void RunOn(JumpSSA &ssa) override {
    auto jump = CopyFromValue(ssa, nullptr);
    CopyOperand(jump, ssa);
  }

  void RunOn(ReturnSSA &ssa) override {
    auto ret = CopyFromValue(ssa, nullptr);
    CopyOperand(ret, ssa);
  }

  void RunOn(AllocaSSA &ssa) override {
    CopyFromValue(ssa);
  }

  void RunOn(ConstIntSSA &ssa) override {
    CopyFromValue(ssa, ssa.value());
  }

  void RunOn(ConstStrSSA &ssa) override {
    CopyFromValue(ssa, ssa.str());
  }

  void RunOn(ConstStructSSA &ssa) override {
    auto const_struct = CopyFromValue(ssa, SSAPtrList(ssa.size()));
    CopyOperand(const_struct, ssa);
  }

  void RunOn(ConstArraySSA &ssa) override {
    auto const_array = CopyFromValue(ssa, SSAPtrList(ssa.size()));
    CopyOperand(const_array, ssa);
  }

  void RunOn(ConstZeroSSA &ssa) override {
    CopyFromValue(ssa);
  }

  void RunOn(PhiOperandSSA &ssa) override {
    auto phi_opr = CopyFromValue(ssa, nullptr, nullptr);
    CopyOperand(phi_opr, ssa);
  }

  void RunOn(PhiSSA &ssa) override {
    auto phi = CopyFromValue(ssa, SSAPtrList(ssa.size()));
    CopyOperand(phi, ssa);
  }

  void RunOn(SelectSSA &ssa) override {
    auto select = CopyFromValue(ssa, nullptr, nullptr, nullptr);
    CopyOperand(select, ssa);
  }

  void RunOn(UndefSSA &ssa) override {
    CopyFromValue(ssa);
  }

  // getters
  const std::list<BlockPtr> &blocks() const { return blocks_; }
  const BlockPtr &entry() const { return entry_; }
  const BlockPtr &exit() const { return exit_; }
  const SSAPtr &ret_val() const { return ret_val_; }

 private:
  SSAPtr GetCopy(const SSAPtr &value) {
    if (!value) return nullptr;
    // find from cache
    auto it = copied_vals_.find(value.get());
    if (it != copied_vals_.end()) {
      return it->second;
    }
    else if (IsSSA<GlobalVarSSA>(value) || IsSSA<FunctionSSA>(value)) {
      // skip global variable and functions
      return copied_vals_[value.get()] = value;
    }
    else if (auto arg = SSADynCast<ArgRefSSA>(value.get())) {
      // return stored argument
      return copied_vals_[value.get()] = args_[arg->index()];
    }
    else {
      value->RunPass(*this);
      auto it = copied_vals_.find(value.get());
      assert(it != copied_vals_.end());
      return it->second;
    }
  }

  template <typename T, typename... Args>
  inline std::shared_ptr<T> CopyFromValue(T &ssa, Args &&... args) {
    auto val = std::make_shared<T>(std::forward<Args>(args)...);
    val->set_logger(ssa.logger());
    val->set_type(ssa.type());
    copied_vals_[&ssa] = val;
    return val;
  }

  void CopyOperand(const UserPtr &copied, User &user) {
    for (std::size_t i = 0; i < user.size(); ++i) {
      (*copied)[i].set_value(GetCopy(user[i].value()));
    }
  }

  // copied blocks
  std::list<BlockPtr> blocks_;
  // pointer to copied entry block & exit block
  BlockPtr entry_, exit_;
  // return value of current function
  SSAPtr ret_val_;
  // current function
  FuncPtr cur_func_;
  // entry block of current function
  BlockSSA *cur_entry_;
  // argument list
  std::vector<SSAPtr> args_;
  // all copied values (source -> copied)
  std::unordered_map<Value *, SSAPtr> copied_vals_;
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

  bool RunOnFunction(const UserPtr &func) override {
    auto func_ptr = SSACast<FunctionSSA>(func);
    if (func_ptr->is_decl()) return false;
    // traverse all instructions
    bool changed = true;
    while (changed) {
      changed = false;
      for (const auto &i : *func) {
        auto block = SSACast<BlockSSA>(i.value());
        if (ScanBlock(func_ptr, block)) {
          changed = true;
          break;
        }
      }
    }
    // release resources
    func_info_.clear();
    return changed;
  }

 private:
  struct FuncInfo {
    // instruction count
    std::size_t inst_count;
    // count of contained function calls (including recursive call)
    std::size_t func_call_count;
    // function is recursive
    bool is_recursive;
  };

  void UpdateFuncInfo(FunctionSSA *func);
  const FuncInfo &GetFuncInfo(FunctionSSA *func);
  bool CanInline(FunctionSSA *cur, CallSSA *call);
  void IncreaseInlineCount(CallSSA *call);
  bool ScanBlock(const FuncPtr &func, const BlockPtr &block);

  // function information
  std::unordered_map<FunctionSSA *, FuncInfo> func_info_;
  // inline counter
  // NOTE: the data will not be cleared between pass
  std::unordered_map<FunctionSSA *, std::size_t> inline_count_;
};

}  // namespace

// register current pass
REGISTER_PASS(FunctionInliningPass, inliner, 2, PassStage::Opt);


// regather & update information of the specific function
void FunctionInliningPass::UpdateFuncInfo(FunctionSSA *func) {
  auto &info = func_info_[func];
  for (const auto &block : *func) {
    auto block_ptr = SSACast<BlockSSA>(block.value().get());
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

// check if target function of call instruction
// can be inlined into current function
bool FunctionInliningPass::CanInline(FunctionSSA *cur, CallSSA *call) {
  auto target = SSACast<FunctionSSA>(call->callee().get());
  if (target->is_decl()) return false;
  const auto &cur_info = GetFuncInfo(cur);
  const auto &target_info = GetFuncInfo(target);
  // do not inline if is calling another recursive function
  // do not inline another function if current is recursive
  if (cur != target &&
      (cur_info.is_recursive || target_info.is_recursive)) {
    return false;
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
  else if (target_info.inst_count > kCalleeInstThreshold ||
           cur_info.inst_count > kCallerInstThreshold) {
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
