#include "define/ast.h"

#include "mid/irbuilder.h"

using namespace mimic::define;
using namespace mimic::mid;


SSAPtr VarDeclAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr VarDefAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr InitListAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr FuncDeclAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr FuncDefAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr FuncParamAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr StructDefAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr EnumDefAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr TypeAliasAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr StructElemAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr StructElemDefAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr EnumElemAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr BlockAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr IfElseAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr WhileAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr ControlAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr BinaryAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr CastAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr UnaryAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr IndexAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr FuncCallAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr AccessAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr IntAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr CharAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr StringAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr IdAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr PrimTypeAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr StructTypeAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr EnumTypeAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr ConstTypeAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr PointerTypeAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}

SSAPtr UserTypeAST::GenerateIR(IRBuilder &irb) {
  return irb.GenerateOn(*this);
}
