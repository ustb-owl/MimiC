#include "back/asm/arch/aarch32/instgen.h"

#include <cmath>

#include "opt/passes/helper/cast.h"
#include "opt/passes/helper/blkiter.h"

#include "xstl/guard.h"

using namespace mimic::define;
using namespace mimic::mid;
using namespace mimic::opt;
using namespace mimic::back::asmgen;
using namespace mimic::back::asmgen::aarch32;

namespace {

using RegName = AArch32Reg::RegName;
using OpCode = AArch32Inst::OpCode;

}  // namespace

// linkage type conversion
InstGenBase::LinkageTypes AArch32InstGen::GetLinkType(
    mimic::mid::LinkageTypes link) {
  using MidLink = mimic::mid::LinkageTypes;
  using GenLink = InstGenBase::LinkageTypes;
  switch (link) {
    case MidLink::Internal: case MidLink::Inline: return GenLink::Internal;
    case MidLink::GlobalCtor: return GenLink::Ctor;
    case MidLink::GlobalDtor: return GenLink::Dtor;
    default: return GenLink::External;
  }
}

OprPtr AArch32InstGen::GenerateZeros(const TypePtr &type) {
  if (arr_depth_) {
    // just generate immediate number
    // 'GenerateOn(ConstArraySSA)' will handle
    return GetImm(0);
  }
  else if (!in_global_ && (type->IsInteger() || type->IsPointer())) {
    return GetImm(0);
  }
  else {
    // generate memory data
    OprPtr label = nullptr;
    if (!in_global_) label = label_fact_.GetLabel();
    auto mem = !in_global_ ? EnterMemData(label, LinkageTypes::Internal)
                           : xstl::Guard(nullptr);
    PushInst(OpCode::ZERO, std::make_shared<AArch32Int>(type->GetSize()));
    return label;
  }
}

void AArch32InstGen::LoadEffAddr(const OprPtr &dest_reg, const OprPtr &ptr,
                                 const OprPtr &offset) {
  // check if 'offset' is immediate zero
  bool ofs_zero = offset->IsImm() &&
                  !static_cast<AArch32Imm *>(offset.get())->val();
  if (ptr->IsSlot()) {
    // get base register
    auto p = static_cast<AArch32Slot *>(ptr.get());
    auto base = p->based_on_sp() ? GetReg(RegName::SP) :
                                   GetReg(RegName::R11);
    PushInst(OpCode::MOV, dest_reg, base);
    // calculate stack offset
    auto ofs = p->offset();
    if (ofs) PushInst(OpCode::ADD, dest_reg, dest_reg, GetImm(ofs));
  }
  else if (ptr->IsLabel()) {
    // load label address
    PushInst(OpCode::LDR, dest_reg, ptr);
  }
  else {
    assert(ptr->IsReg());
    // just move address to destination
    PushInst(OpCode::MOV, dest_reg, ptr);
  }
  // add offset to result if offset is not zero
  if (!ofs_zero) PushInst(OpCode::ADD, dest_reg, dest_reg, offset);
}

void AArch32InstGen::GenerateMemCpy(const OprPtr &dest, const OprPtr &src,
                                    std::size_t size) {
  LoadEffAddr(GetReg(RegName::R0), dest, GetImm(0));
  LoadEffAddr(GetReg(RegName::R1), src, GetImm(0));
  PushInst(OpCode::MOV, GetReg(RegName::R2), GetImm(size));
  PushInst(OpCode::BL, label_fact_.GetLabel("memcpy"));
}

void AArch32InstGen::DumpSeqs(std::ostream &os,
                              const InstSeqMap &seqs) const {
  for (const auto &[label, info] : seqs) {
    // dump '.globl' if is global
    if (info.link != LinkageTypes::Internal) {
      os << '\t' << ".globl ";
      label->Dump(os);
      os << std::endl;
    }
    // dump label
    label->Dump(os);
    os << ':' << std::endl;
    // dump instructions
    for (const auto &i : info.insts) i->Dump(os);
    os << std::endl;
  }
}

OprPtr AArch32InstGen::GenerateOn(LoadSSA &ssa) {
  OprPtr dest, src = GetOpr(ssa.ptr());
  const auto &type = ssa.type();
  if (type->IsArray() || type->IsStruct()) {
    assert(src->IsLabel() || src->IsSlot());
    // generate 'memcpy'
    auto size = ssa.type()->GetSize();
    dest = AllocNextSlot(cur_label(), size);
    GenerateMemCpy(dest, src, size);
  }
  else if (IsSSA<AllocaSSA>(ssa.ptr())) {
    assert(src->IsReg());
    // generate move
    dest = vreg_fact_.GetReg();
    PushInst(OpCode::MOV, dest, src);
  }
  else {
    assert(src->IsReg() || src->IsLabel());
    // load address to register if source operand is a label
    if (src->IsLabel()) {
      auto temp = GetReg(RegName::R0);
      LoadEffAddr(temp, src, GetImm(0));
      src = temp;
    }
    // generate memory load
    dest = vreg_fact_.GetReg();
    auto opcode = type->GetSize() == 1 ? OpCode::LDRB : OpCode::LDR;
    PushInst(opcode, dest, src);
  }
  return dest;
}

OprPtr AArch32InstGen::GenerateOn(StoreSSA &ssa) {
  auto ptr = GetOpr(ssa.ptr()), val = GetOpr(ssa.value());
  const auto &type = ssa.value()->type();
  if (type->IsArray() || type->IsStruct()) {
    assert((ptr->IsLabel() || ptr->IsSlot() || ptr->IsVirtual()) &&
           (val->IsLabel() || val->IsSlot() || ptr->IsVirtual()));
    // generate 'memcpy'
    auto size = type->GetSize();
    GenerateMemCpy(ptr, val, size);
  }
  else if (IsSSA<AllocaSSA>(ssa.ptr())) {
    assert(ptr->IsReg());
    // generate move
    PushInst(OpCode::MOV, ptr, val);
  }
  else {
    assert((ptr->IsReg() || ptr->IsLabel()) &&
           (val->IsReg() || val->IsImm()));
    // generate pointer register
    auto ptr_reg = ptr;
    if (ptr->IsLabel()) {
      ptr_reg = GetReg(RegName::R0);
      PushInst(OpCode::LDR, ptr_reg, ptr);
    }
    // generate memory store
    auto opcode = type->GetSize() == 1 ? OpCode::STRB : OpCode::STR;
    PushInst(opcode, val, ptr_reg);
  }
  return nullptr;
}

OprPtr AArch32InstGen::GenerateOn(AccessSSA &ssa) {
  using AccTy = AccessSSA::AccessType;
  auto ptr = GetOpr(ssa.ptr()), index = GetOpr(ssa.index());
  auto dest = vreg_fact_.GetReg();
  // calculate index
  auto base_ty = ssa.ptr()->type()->GetDerefedType();
  if (base_ty->IsStruct()) {
    assert(ssa.acc_type() == AccTy::Element);
    // structures
    std::size_t idx = SSACast<ConstIntSSA>(ssa.index().get())->value();
    std::size_t offset = 0, align = base_ty->GetAlignSize();
    for (std::size_t i = 0; i < idx; ++i) {
      auto k = (base_ty->GetElem(i)->GetSize() + align - 1) / align;
      offset += align * k;
    }
    index = GetImm(offset);
  }
  else {
    // pointers or arrays
    // check if is array accessing
    if (ssa.acc_type() == AccTy::Element) {
      assert(base_ty->IsArray());
      base_ty = base_ty->GetDerefedType();
    }
    // get offset by size of base type
    auto size = base_ty->GetSize();
    if (ssa.index()->IsConst()) {
      auto ofs = SSACast<ConstIntSSA>(ssa.index().get())->value() * size;
      index = GetImm(ofs);
    }
    else {
      assert(index->IsReg() && size);
      auto temp = GetReg(RegName::R0);
      if (!(size & (size - 1))) {
        // 'size' is not zero && is power of 2
        PushInst(OpCode::LSL, temp, index, GetImm(std::log2(size)));
      }
      else {
        // generate multiplication
        PushInst(OpCode::MUL, temp, index, GetImm(size));
      }
      index = temp;
    }
  }
  // get effective address
  LoadEffAddr(dest, ptr, index);
  return dest;
}

OprPtr AArch32InstGen::GenerateOn(BinarySSA &ssa) {
  using Op = BinarySSA::Operator;
  auto lhs = GetOpr(ssa.lhs()), rhs = GetOpr(ssa.rhs());
  auto dest = vreg_fact_.GetReg(), temp = GetReg(RegName::R0);
  // get opcode by operator
  OpCode opcode;
  switch (ssa.op()) {
    // simple operations
    case Op::Add: opcode = OpCode::ADD; break;
    case Op::Sub: opcode = OpCode::SUB; break;
    case Op::Mul: opcode = OpCode::MUL; break;
    case Op::UDiv: opcode = OpCode::UDIV; break;
    case Op::SDiv: opcode = OpCode::SDIV; break;
    case Op::And: opcode = OpCode::AND; break;
    case Op::Or: opcode = OpCode::ORR; break;
    case Op::Xor: opcode = OpCode::EOR; break;
    case Op::Shl: opcode = OpCode::LSL; break;
    case Op::LShr: opcode = OpCode::LSR; break;
    case Op::AShr: opcode = OpCode::ASR; break;
    // complex operations
    case Op::URem: case Op::SRem: {
      auto opcode = ssa.op() == Op::URem ? OpCode::UDIV : OpCode::SDIV;
      PushInst(opcode, temp, lhs, rhs);
      PushInst(OpCode::MLS, dest, temp, rhs, lhs);
      return dest;
    }
    case Op::Equal: {
      PushInst(OpCode::SUB, temp, lhs, rhs);
      PushInst(OpCode::CLZ, temp, temp);
      PushInst(OpCode::LSR, dest, temp, GetImm(5));
      return dest;
    }
    case Op::NotEq: {
      PushInst(OpCode::SUBS, dest, lhs, rhs);
      PushInst(OpCode::MOVWNE, dest, GetImm(1));
      return dest;
    }
    case Op::ULess: case Op::SLess: case Op::ULessEq: case Op::SLessEq:
    case Op::UGreat: case Op::SGreat: case Op::UGreatEq:
    case Op::SGreatEq: {
      PushInst(OpCode::MOV, dest, GetImm(0));
      PushInst(OpCode::CMP, lhs, rhs);
      OpCode opcode;
      switch (ssa.op()) {
        case Op::ULess: opcode = OpCode::MOVWLO; break;
        case Op::SLess: opcode = OpCode::MOVWLT; break;
        case Op::ULessEq: opcode = OpCode::MOVWLS; break;
        case Op::SLessEq: opcode = OpCode::MOVWLE; break;
        case Op::UGreat: opcode = OpCode::MOVWHI; break;
        case Op::SGreat: opcode = OpCode::MOVWGT; break;
        case Op::UGreatEq: opcode = OpCode::MOVWHS; break;
        case Op::SGreatEq: opcode = OpCode::MOVWGE; break;
        default: assert(false);
      }
      PushInst(opcode, dest, GetImm(1));
      return dest;
    }
    default: assert(false);
  }
  // generate binary operation
  PushInst(opcode, dest, lhs, rhs);
  return dest;
}

OprPtr AArch32InstGen::GenerateOn(UnarySSA &ssa) {
  using Op = UnarySSA::Operator;
  auto opr = GetOpr(ssa.opr()), dest = vreg_fact_.GetReg();
  switch (ssa.op()) {
    case Op::Neg: PushInst(OpCode::RSB, dest, opr, GetImm(0)); break;
    case Op::LogicNot: {
      auto temp = GetReg(RegName::R0);
      PushInst(OpCode::CLZ, temp, opr);
      PushInst(OpCode::LSR, dest, temp, GetImm(5));
      break;
    }
    case Op::Not: PushInst(OpCode::MVN, dest, opr); break;
    default: assert(false);
  }
  return dest;
}

OprPtr AArch32InstGen::GenerateOn(CastSSA &ssa) {
  auto dest = vreg_fact_.GetReg(), opr = GetOpr(ssa.opr());
  const auto &src_ty = ssa.opr()->type(), &dst_ty = ssa.type();
  if (src_ty->GetSize() < dst_ty->GetSize()) {
    assert(src_ty->GetSize() == 1 && dst_ty->GetSize() == 4);
    // unsigned/signed extending
    auto opcode = src_ty->IsUnsigned() || src_ty->IsPointer()
                      ? OpCode::UXTB
                      : OpCode::SXTB;
    PushInst(opcode, dest, opr);
  }
  else if (src_ty->GetSize() > dst_ty->GetSize()) {
    assert(src_ty->GetSize() == 4 && dst_ty->GetSize() == 1);
    // truncating
    PushInst(OpCode::AND, dest, opr, GetImm(255));
  }
  else {
    // maybe pointer to pointer cast
    if (opr->IsLabel() || opr->IsSlot()) {
      assert(src_ty->IsPointer() && dst_ty->IsPointer());
      // load address to dest
      LoadEffAddr(dest, opr, GetImm(0));
    }
    else {
      assert(opr->IsReg() || opr->IsImm());
      // just generate a move
      PushInst(OpCode::MOV, dest, opr);
    }
  }
  return dest;
}

OprPtr AArch32InstGen::GenerateOn(CallSSA &ssa) {
  // generate arguments
  for (std::size_t i = 1; i < ssa.size(); ++i) {
    // generate destination
    OprPtr dest;
    switch (i) {
      case 1: dest = GetReg(RegName::R0); break;
      case 2: dest = GetReg(RegName::R1); break;
      case 3: dest = GetReg(RegName::R2); break;
      case 4: dest = GetReg(RegName::R3); break;
      default: dest = GetSlot(true, (i - 5) * 4); break;
    }
    // put value to destination
    auto val = GetOpr(ssa[i].value());
    if (i <= 4) {
      PushInst(OpCode::MOV, dest, val);
    }
    else {
      PushInst(OpCode::STR, val, dest);
    }
  }
  // generate branch
  PushInst(OpCode::BL, GetOpr(ssa.callee()));
  // generate result
  if (!ssa.type()->IsVoid()) {
    auto dest = vreg_fact_.GetReg();
    PushInst(OpCode::MOV, dest, GetReg(RegName::R0));
    return dest;
  }
  else {
    return nullptr;
  }
}

OprPtr AArch32InstGen::GenerateOn(BranchSSA &ssa) {
  // generate condition
  PushInst(OpCode::CMP, GetOpr(ssa.cond()), GetImm(0));
  // generate branchs
  PushInst(OpCode::BEQ, GetOpr(ssa.false_block()));
  PushInst(OpCode::B, GetOpr(ssa.true_block()));
  return nullptr;
}

OprPtr AArch32InstGen::GenerateOn(JumpSSA &ssa) {
  // generate directly jump
  PushInst(OpCode::B, GetOpr(ssa.target()));
  return nullptr;
}

OprPtr AArch32InstGen::GenerateOn(ReturnSSA &ssa) {
  // generate return value
  if (ssa.value()) {
    PushInst(OpCode::MOV, GetReg(RegName::R0), GetOpr(ssa.value()));
  }
  // generate jump
  PushInst(OpCode::BX, GetReg(RegName::LR));
  return nullptr;
}

OprPtr AArch32InstGen::GenerateOn(FunctionSSA &ssa) {
  auto label = label_fact_.GetLabel(ssa.name());
  // skip all declarations
  if (ssa.is_decl()) return label;
  // enter a new function
  auto func = EnterFunction(label, GetLinkType(ssa.link()));
  ssa.set_metadata(label);
  // generate arguments
  args_.clear();
  for (std::size_t i = 0; i < ssa.args().size(); ++i) {
    OprPtr arg = vreg_fact_.GetReg(), src;
    // get source of arguments
    switch (i) {
      case 0: src = GetReg(RegName::R0); break;
      case 1: src = GetReg(RegName::R1); break;
      case 2: src = GetReg(RegName::R2); break;
      case 3: src = GetReg(RegName::R3); break;
      default: src = GetSlot((i - 4) * 4); break;
    }
    // put argument to register
    PushInst(i < 4 ? OpCode::MOV : OpCode::LDR, arg, src);
    args_.push_back(std::move(arg));
  }
  // generate label of blocks
  for (const auto &i : ssa) {
    i.value()->set_metadata(label_fact_.GetLabel());
  }
  // generate all blocks in BFS order
  auto entry = SSACast<BlockSSA>(ssa.entry().get());
  for (const auto &i : BFSTraverse(entry)) GenerateCode(*i);
  return label;
}

OprPtr AArch32InstGen::GenerateOn(GlobalVarSSA &ssa) {
  auto label = label_fact_.GetLabel(ssa.name());
  auto mem = EnterMemData(label, GetLinkType(ssa.link()));
  // generate initial value
  if (ssa.init()) {
    ++in_global_;
    GetOpr(ssa.init());
    --in_global_;
  }
  else {
    // generate zeros
    auto size = ssa.type()->GetDerefedType()->GetSize();
    PushInst(OpCode::ZERO, std::make_shared<AArch32Int>(size));
  }
  return label;
}

OprPtr AArch32InstGen::GenerateOn(AllocaSSA &ssa) {
  auto type = ssa.type()->GetDerefedType();
  if (type->IsArray() || type->IsStruct()) {
    // allocate a stack slot
    return AllocNextSlot(cur_label(), type->GetSize());
  }
  else {
    // allocate a virtual register
    return vreg_fact_.GetReg();
  }
}

OprPtr AArch32InstGen::GenerateOn(BlockSSA &ssa) {
  // generate label
  assert(ssa.metadata().has_value());
  PushInst(OpCode::LABEL, GetOpr(ssa));
  // generate instructions
  for (const auto &i : ssa.insts()) GenerateCode(i);
  return nullptr;
}

OprPtr AArch32InstGen::GenerateOn(ArgRefSSA &ssa) {
  return args_[ssa.index()];
}

OprPtr AArch32InstGen::GenerateOn(ConstIntSSA &ssa) {
  if (in_global_) {
    auto opcode = ssa.type()->GetSize() == 1 ? OpCode::BYTE : OpCode::LONG;
    PushInst(opcode, std::make_shared<AArch32Int>(ssa.value()));
    return nullptr;
  }
  else {
    return GetImm(ssa.value());
  }
}

OprPtr AArch32InstGen::GenerateOn(ConstStrSSA &ssa) {
  OprPtr label = nullptr;
  if (!in_global_) label = label_fact_.GetLabel();
  auto mem = !in_global_ ? EnterMemData(label, LinkageTypes::Internal)
                         : xstl::Guard(nullptr);
  PushInst(OpCode::ASCIZ, std::make_shared<AArch32Str>(ssa.str()));
  return label;
}

OprPtr AArch32InstGen::GenerateOn(ConstStructSSA &ssa) {
  // not implemented
  assert(false);
}

OprPtr AArch32InstGen::GenerateOn(ConstArraySSA &ssa) {
  ++arr_depth_;
  auto guard = xstl::Guard([this] { --arr_depth_; });
  // generate label
  OprPtr label = nullptr;
  bool new_data = !in_global_ && arr_depth_ <= 1;
  if (new_data) label = label_fact_.GetLabel();
  auto mem = new_data ? EnterMemData(label, LinkageTypes::Internal)
                      : xstl::Guard(nullptr);
  // generate elements
  std::size_t zeros = 0;
  for (const auto &i : ssa) {
    auto val = GetOpr(i.value());
    if (!val) continue;
    if (val->IsImm()) {
      auto size = i.value()->type()->GetSize();
      auto int_val = static_cast<AArch32Imm *>(val.get())->val();
      // combine all zeros
      if (!int_val) {
        zeros += size;
        continue;
      }
      else if (zeros) {
        PushInst(OpCode::ZERO, std::make_shared<AArch32Int>(zeros));
        zeros = 0;
      }
      // convert to aarch32 integer
      val = std::make_shared<AArch32Int>(int_val);
      PushInst(size == 1 ? OpCode::BYTE : OpCode::LONG, val);
    }
    else {
      // label or other things
      PushInst(OpCode::LONG, val);
    }
  }
  // handle the rest zeros
  if (zeros) PushInst(OpCode::ZERO, std::make_shared<AArch32Int>(zeros));
  return label;
}

OprPtr AArch32InstGen::GenerateOn(ConstZeroSSA &ssa) {
  return GenerateZeros(ssa.type());
}

OprPtr AArch32InstGen::GenerateOn(SelectSSA &ssa) {
  auto dest = vreg_fact_.GetReg(), cond = GetOpr(ssa.cond());
  auto tv = GetOpr(ssa.true_val()), fv = GetOpr(ssa.false_val());
  PushInst(OpCode::MOV, dest, tv);
  PushInst(OpCode::CMP, cond, GetImm(0));
  PushInst(OpCode::MOVEQ, dest, fv);
  return dest;
}

OprPtr AArch32InstGen::GenerateOn(UndefSSA &ssa) {
  // treat undefined value as zero
  return GenerateZeros(ssa.type());
}

void AArch32InstGen::Dump(std::ostream &os) const {
  // dump all functions
  os << "\t.text" << std::endl;
  DumpSeqs(os, funcs());
  // dump all memory data
  if (!mems().empty()) {
    os << "\t.data" << std::endl;
    DumpSeqs(os, mems());
  }
}

void AArch32InstGen::Reset() {
  // clear all maps
  regs_.clear();
  imms_.clear();
  slots_.clear();
  // initialize all registers
  for (int i = 0; i < 16; ++i) {
    auto name = static_cast<RegName>(i);
    auto reg = std::make_shared<AArch32Reg>(name);
    regs_.insert({name, reg});
  }
  // reset other stuffs
  alloc_slots_.clear();
  args_.clear();
  in_global_ = 0;
  arr_depth_ = 0;
}

SlotAllocator AArch32InstGen::GetSlotAllocator() {
  return SlotAllocator([this](const OprPtr &func_label) {
    return AllocNextSlot(func_label, 4);
  });
}
