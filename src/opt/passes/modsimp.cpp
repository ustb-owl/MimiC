
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include "mid/module.h"
#include "mid/ssa.h"
#include "opt/pass.h"
#include "opt/passman.h"

using namespace mimic::opt;
using namespace mimic::mid;
using namespace mimic::define;

namespace {

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#define LOG() \
  std::cout << '[' << __FILENAME__ << ":" << std::dec << __LINE__ << "] "

class ModSimplication : public BlockPass {
 public:
  ModSimplication() = default;

  bool RunOnBlock(const BlockPtr& block) override {
    changed_ = replace_ = false;
    auto instIter = block->insts().begin();
    auto instEnd = block->insts().end();

    for (; instIter != instEnd; instIter++) {
      finalSSA_ = nullptr;
      mod = MakeModule((*instIter)->logger(), block, instIter);
      (*instIter)->RunPass(*this);

      if (replace_) {
        (*instIter)->ReplaceBy(finalSSA_);
        *instIter = finalSSA_;
        finalSSA_ = nullptr;
        replace_ = false;
      }
    }

    return changed_;
  }

  void RunOn(BinarySSA& ssa) override {
    const SSAPtr& left = ssa.lhs();
    const SSAPtr& right = ssa.rhs();

    if (ssa.op() == BinarySSA::Operator::URem) {
      auto tmpMod = MakeModule(ssa.logger());
      auto divSSA = mod.CreateDiv(left, right);
      auto mulSSA = mod.CreateMul(right, divSSA);
      auto subSSA = tmpMod.CreateSub(left, mulSSA);
      finalSSA_ = subSSA;
      changed_ = replace_ = true;
    }
    else if (ssa.op() == BinarySSA::Operator::SRem) {
      auto tmpMod = MakeModule(ssa.logger());
      auto divSSA = mod.CreateBinary(BinarySSA::Operator::SDiv, left, right,
                                     ssa.type());
      auto mulSSA = mod.CreateMul(right, divSSA);
      auto subSSA = tmpMod.CreateSub(left, mulSSA);
      finalSSA_ = subSSA;
      changed_ = replace_ = true;
    }
  }

 private:
  static inline TypePtr GenIntType() {
    return MakePrimType(PrimType::Type::Int32, false);
  }

  Module mod;
  bool changed_, replace_;
  SSAPtr finalSSA_ = nullptr;
  std::optional<std::uint32_t> operand_;
};

}  // namespace

REGISTER_PASS(ModSimplication, modsimp, 1,
              PassStage::PreOpt | PassStage::Opt | PassStage::PostOpt);