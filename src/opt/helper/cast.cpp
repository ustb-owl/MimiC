#include "opt/helper/cast.h"

using namespace mimic::mid;
using namespace mimic::opt::__impl;

IsSSAHelperPass::SSAType IsSSAHelperPass::type_;

IsSSAHelperPass &IsSSAHelperPass::GetInstance() {
  static IsSSAHelperPass helper;
  return helper;
}

void IsSSAHelperPass::RunOn(LoadSSA &ssa) {
  type_ = SSAType::Load;
}

void IsSSAHelperPass::RunOn(StoreSSA &ssa) {
  type_ = SSAType::Store;
}

void IsSSAHelperPass::RunOn(AccessSSA &ssa) {
  type_ = SSAType::Access;
}

void IsSSAHelperPass::RunOn(BinarySSA &ssa) {
  type_ = SSAType::Binary;
}

void IsSSAHelperPass::RunOn(UnarySSA &ssa) {
  type_ = SSAType::Unary;
}

void IsSSAHelperPass::RunOn(CastSSA &ssa) {
  type_ = SSAType::Cast;
}

void IsSSAHelperPass::RunOn(CallSSA &ssa) {
  type_ = SSAType::Call;
}

void IsSSAHelperPass::RunOn(BranchSSA &ssa) {
  type_ = SSAType::Branch;
}

void IsSSAHelperPass::RunOn(JumpSSA &ssa) {
  type_ = SSAType::Jump;
}

void IsSSAHelperPass::RunOn(ReturnSSA &ssa) {
  type_ = SSAType::Return;
}

void IsSSAHelperPass::RunOn(FunctionSSA &ssa) {
  type_ = SSAType::Function;
}

void IsSSAHelperPass::RunOn(GlobalVarSSA &ssa) {
  type_ = SSAType::GlobalVar;
}

void IsSSAHelperPass::RunOn(AllocaSSA &ssa) {
  type_ = SSAType::Alloca;
}

void IsSSAHelperPass::RunOn(BlockSSA &ssa) {
  type_ = SSAType::Block;
}

void IsSSAHelperPass::RunOn(ArgRefSSA &ssa) {
  type_ = SSAType::ArgRef;
}

void IsSSAHelperPass::RunOn(ConstIntSSA &ssa) {
  type_ = SSAType::ConstInt;
}

void IsSSAHelperPass::RunOn(ConstStrSSA &ssa) {
  type_ = SSAType::ConstStr;
}

void IsSSAHelperPass::RunOn(ConstStructSSA &ssa) {
  type_ = SSAType::ConstStruct;
}

void IsSSAHelperPass::RunOn(ConstArraySSA &ssa) {
  type_ = SSAType::ConstArray;
}

void IsSSAHelperPass::RunOn(ConstZeroSSA &ssa) {
  type_ = SSAType::ConstZero;
}

void IsSSAHelperPass::RunOn(PhiOperandSSA &ssa) {
  type_ = SSAType::PhiOperand;
}

void IsSSAHelperPass::RunOn(PhiSSA &ssa) {
  type_ = SSAType::Phi;
}

void IsSSAHelperPass::RunOn(SelectSSA &ssa) {
  type_ = SSAType::Select;
}

void IsSSAHelperPass::RunOn(UndefSSA &ssa) {
  type_ = SSAType::Undef;
}
