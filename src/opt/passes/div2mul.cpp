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

class Multiplier {
 public:
  Multiplier(uint64_t M, int L) : m(M), l(L) {}
  uint64_t m;
  int l;
};

class Div2mul : public BlockPass {
  enum class SSAOperationType { Replace, Create, Nothing };

 public:
  Div2mul() = default;

  bool RunOnBlock(const BlockPtr &block) override {
    changed_ = false;
    auto instIter = block->insts().begin();
    auto instEnd = block->insts().end();

    for (; instIter != instEnd; instIter++) {
      auto it = *instIter;
      finalSSA_ = nullptr;
      currentIter_ = instIter;

      ssaOperationType = SSAOperationType::Nothing;
      it->RunPass(*this);
      if (ssaOperationType == SSAOperationType::Replace) {
        it->ReplaceBy(finalSSA_);
        it = finalSSA_;
      }
      else {
        continue;
      }
    }
    return changed_;
  }

  void RunOn(BinarySSA &ssa) override {
    auto mod = MakeModule(ssa.logger());
    SSAPtr left, right;
    left = ssa.lhs();
    right = ssa.rhs();
    //    /*
    operand_ = {};
    right->RunPass(*this);
    if (operand_ && (ssa.op() == BinarySSA::Operator::UDiv)) {
      // collect divisor

      if (operand_ == 0) {
        ssa.logger()->LogWarning(
            "ZeroDivisionError: integer division or modulo by zero");
        ssaOperationType = SSAOperationType::Nothing;
        return;
      }
      //      LOG() << "Hi1\n";
      if (operand_ >= (uint32_t(1) << (N - 1))) {
        //        LOG() << "Hi\n";
        ssaOperationType = SSAOperationType::Replace;
        auto uGreatEqSSA =
            mod.CreateGreatEq(left, mod.GetInt(*operand_, GenIntType()));
        finalSSA_ = uGreatEqSSA;
        changed_ = true;
      }
      else {
        int s = ctz(*operand_);
        if (operand_ == (uint32_t(1) << s)) {
          if (s > 0) {
            LOG() << "return n >> " << s << "\n";
            ssaOperationType = SSAOperationType::Replace;
            auto lshr = mod.CreateShr(left, mod.GetInt(s, GenIntType()));
            finalSSA_ = lshr;
            changed_ = true;
          }
          else {
            LOG() << "return n;\n";
            ssaOperationType = SSAOperationType::Replace;
            finalSSA_ = left;
            changed_ = true;
          }
        }
        // bigger
        else {
          // choose Multiplier
          Multiplier multiplier = chooseMultiplier(*operand_, N);
          if (multiplier.m < (uint64_t(1) << N)) {
            s = 0;
          }
          else {
            multiplier = chooseMultiplier(*operand_ >> s, N - s);
          }

          // optimize
          if (multiplier.m < (uint64_t(1) << N)) {
            if (s > 0) {
              if (multiplier.l > 0) {
                /*
                LOG() << "return muluh(n >> " << s << ", " << multiplier.m
                      << ") >> ;" << multiplier.l << "\n";
                      */
              }
              else {
                /*
                LOG() << "return muluh(n >> " << s << ", " << multiplier.m
                      << ");\n";
              */
              }
            }
            else {
              if (multiplier.l > 0) {
                /*
                LOG() << "return muluh(n, " << multiplier.m << ") >> "
                      << multiplier.l << "\n";
                      */
              }
              else {
                /*
                LOG() << "return muluh(n, " << multiplier.m << ");\n";
                 */
              }
            }
          }
          else {
            LOG() << "    Uint32 t = muluh(n, "
                  << (multiplier.m - (uint64_t(1) << N)) << ");\n";
            LOG() << "    return (((n - t) >> 1) + t) >> "
                  << (multiplier.l - 1) << ";\n";
          }
        }
      }
    }
    //*/
  }

  void RunOn(CastSSA &ssa) override {
    //    LOG() << "In cast\n";
    ssa[0].value()->RunPass(*this);
  }

  void RunOn(ConstIntSSA &ssa) override {
    //    LOG() << "Right: " << ssa.value() << "\n";
    operand_ = ssa.value();
  }

 protected:
  Multiplier chooseMultiplier(uint32_t d, int p) const {
    assert(d != 0);
    assert(p >= 1 && p <= N);
    int l = N - clz(d - 1);
    uint64_t low = (uint64_t(1) << (N + l)) / d;
    uint64_t high =
        ((uint64_t(1) << (N + l)) + (uint64_t(1) << (N + l - p))) / d;
    while ((low >> 1) < (high >> 1) && l > 0) {
      low >>= 1, high >>= 1, --l;
    }
    return {high, l};
  }

 private:
  static inline int clz(int32_t x) { return __builtin_clz(x); }
  static inline int ctz(int32_t x) { return __builtin_ctz(x); }

  static inline TypePtr GenIntType() {
    return MakePrimType(PrimType::Type::UInt32, false);
  }

  const int N = 32;
  bool changed_{};
  SSAPtr finalSSA_ = nullptr;
  SSAPtrList::iterator currentIter_;
  SSAOperationType ssaOperationType = SSAOperationType::Nothing;
  std::optional<std::uint32_t> operand_;
};

}  // namespace

///*
REGISTER_PASS(Div2mul, div2mul, 1,
              PassStage::PreOpt | PassStage::Opt | PassStage::PostOpt);
//              */