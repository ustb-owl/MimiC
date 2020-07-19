#include "mid/ssa.h"

#include <cassert>

#include "back/codegen.h"

using namespace mimic::mid;
using namespace mimic::back;

void LoadSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void StoreSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void AccessSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void BinarySSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void UnarySSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void CastSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void CallSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void BranchSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void JumpSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void ReturnSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void FunctionSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void GlobalVarSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void AllocaSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void BlockSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void ArgRefSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void ConstIntSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void ConstStrSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void ConstStructSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void ConstArraySSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void ConstZeroSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}

void PhiOperandSSA::GenerateCode(CodeGen &gen) {
  // backend do not support phi node
  assert(false);
}

void PhiSSA::GenerateCode(CodeGen &gen) {
  // backend do not support phi node
  assert(false);
}

void SelectSSA::GenerateCode(CodeGen &gen) {
  gen.GenerateOn(*this);
}
