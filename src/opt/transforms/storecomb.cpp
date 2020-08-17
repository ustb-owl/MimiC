#include <unordered_map>
#include <cassert>

#include "opt/pass.h"
#include "opt/passman.h"
#include "opt/helper/cast.h"
#include "opt/helper/const.h"
#include "mid/module.h"

using namespace mimic::mid;
using namespace mimic::opt;

namespace {

/*
  store instruction combination
  this pass will try to combine multiple constant stores to a single store
*/
class StoreCombiningPass : public BlockPass {
 public:
  StoreCombiningPass() {}

  bool RunOnBlock(const BlockPtr &block) override {
    // traverse all instructions
    auto &insts = block->insts();
    for (auto it = insts.begin(); it != insts.end();) {
      if (auto store = SSADynCast<StoreSSA>(it->get())) {
        it = HandleStores(insts, it, *store);
      }
      else if (IsSSA<LoadSSA>(it->get()) || IsSSA<CallSSA>(it->get())) {
        it = CheckAndEmit(insts, it);
      }
      else {
        ++it;
      }
    }
    CheckAndEmit(insts, --insts.end());
    // remove marked stores
    for (const auto &store : removed_stores_) {
      block->insts().remove_if(
          [store](const SSAPtr &i) { return i.get() == store; });
    }
    return !removed_stores_.empty();
  }

  void CleanUp() override {
    removed_stores_.clear();
    arrays_.clear();
  }

 private:
  using ArrayVal = std::unordered_map<std::size_t, SSAPtr>;
  using SSAIt = SSAPtrList::iterator;

  struct ArrayInfo {
    ArrayVal elems;
    std::vector<StoreSSA *> stores;
  };

  SSAIt HandleStores(SSAPtrList &insts, SSAIt pos, StoreSSA &store) {
    // handle constant stores only
    if (!store.value()->IsConst()) return CheckAndEmit(insts, pos);
    // handle if pointer is an access instruction
    auto acc = SSADynCast<AccessSSA>(store.ptr());
    if (!acc) {
      if (IsSSA<CastSSA>(store.ptr())) {
        // avoid pointer alias
        return CheckAndEmit(insts, pos);
      }
      else {
        return ++pos;
      }
    }
    // handle local array access only
    if (!acc->ptr()->type()->GetDerefedType()->IsArray() ||
        !acc->index()->IsConst()) {
      return CheckAndEmit(insts, pos);
    }
    // get index of access
    assert(acc->index()->type()->IsInteger());
    auto index = ConstantHelper::Fold(acc->index())->value();
    // update array info
    auto &info = arrays_[acc->ptr()];
    info.elems[index] = store.value();
    info.stores.push_back(&store);
    return ++pos;
  }

  // check if the specific value is constant zero
  bool IsZero(const SSAPtr &val) {
    assert(val->IsConst());
    if (auto cint = ConstantHelper::Fold(val)) {
      return !cint->value();
    }
    else if (auto carr = SSADynCast<ConstArraySSA>(val.get())) {
      for (const auto &elem : *carr) {
        if (!IsZero(elem.value())) return false;
      }
      return true;
    }
    return false;
  }

  // check all tracked arrays, emit if possible
  SSAIt CheckAndEmit(SSAPtrList &insts, SSAIt pos) {
    for (const auto &[ptr, info] : arrays_) {
      auto arr_ty = ptr->type()->GetDerefedType();
      auto arr_len = arr_ty->GetLength();
      if (info.elems.size() == arr_len) {
        // all elements are constants, can be emitted
        // read all elements to list
        SSAPtrList elems;
        bool all_zero = true;
        for (std::size_t i = 0; i < arr_len; ++i) {
          auto it = info.elems.find(i);
          assert(it != info.elems.end());
          elems.push_back(it->second);
          // check if all elements are zero
          if (all_zero && !IsZero(it->second)) all_zero = false;
        }
        // make constant array
        auto mod = MakeModule(ptr->logger());
        auto carr = all_zero ? mod.GetZero(arr_ty)
                             : mod.GetArray(elems, arr_ty);
        // insert a store instruction before current position
        auto store = mod.CreateStore(carr, ptr);
        pos = ++insts.insert(pos, store);
        // update 'removed_stores_'
        removed_stores_.insert(removed_stores_.end(), info.stores.begin(),
                               info.stores.end());
      }
    }
    arrays_.clear();
    return ++pos;
  }

  // store instructions that should be removed
  std::vector<StoreSSA *> removed_stores_;
  // value of tracked arrays
  std::unordered_map<SSAPtr, ArrayInfo> arrays_;
};

}  // namespace


// register pass
REGISTER_PASS(StoreCombiningPass, store_comb)
    .set_min_opt_level(2)
    .set_stages(PassStage::Opt)
    .Requires("gvn");
