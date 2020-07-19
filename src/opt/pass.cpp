#include "mid/ssa.h"

#include "opt/pass.h"

using namespace mimic::mid;
using namespace mimic::opt;

void LoadSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void StoreSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void AccessSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void BinarySSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void UnarySSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void CastSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void CallSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void BranchSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void JumpSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void ReturnSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void FunctionSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void GlobalVarSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void AllocaSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void BlockSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void ArgRefSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void ConstIntSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void ConstStrSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void ConstStructSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void ConstArraySSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void ConstZeroSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void PhiOperandSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void PhiSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}

void SelectSSA::RunPass(PassBase &pass) {
  pass.RunOn(*this);
}
