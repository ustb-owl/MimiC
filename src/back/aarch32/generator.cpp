#include "back/aarch32/generator.h"

using namespace mimic::back;
using namespace mimic::back::aarch32;
using namespace mimic::define;
using namespace mimic::mid;

namespace {

using Asm = ARMv7Asm;
using Opcode = ARMv7Opcode;
using Reg = ARMv7Reg;

const char *kIndent = "\t", *kLabelPrefix = ".LBB_";
const char *kDiv = "__aeabi_idiv", *kRem = "__aeabi_idivmod";
const char *kUDiv = "__aeabi_uidiv", *kURem = "__aeabi_uidivmod";


inline std::string_view GetLinkType(LinkageTypes link) {
  switch (link) {
    case LinkageTypes::Internal:
      return "static ";
    case LinkageTypes::Inline:
      return "inline ";
    case LinkageTypes::GlobalCtor:
      return "__attribute((constructor)) ";
    case LinkageTypes::GlobalDtor:
      return "__attribute((destructor)) ";
    default:
      return "";
  }
}

// inline std::size_t RoundUpTo8(std::size_t num) {
//   return ((num + 7) / 8) * 8;
// }

}  // namespace

std::string ARMCodeGen::GetNewVar(const char *prefix,
                                  bool without_prefix = false) {
  return without_prefix ? std::to_string(counter_++)
                        : prefix + std::to_string(counter_++);
}

const std::string &ARMCodeGen::GetLabel(const mid::SSAPtr &ssa) {
  auto val = std::any_cast<std::string>(&ssa->metadata());
  if (!val) {
    ssa->set_metadata(GetNewVar(kLabelPrefix));
    val = std::any_cast<std::string>(&ssa->metadata());
  }
  return *val;
}

std::string ARMCodeGen::GetTypeName(const TypePtr &type) {
  if (type->IsVoid()) {
    // just void
    return "void";
  }
  else if (type->IsInteger()) {
    // integers
    if (type->GetSize() == 1) {
      return type->IsUnsigned() ? "unsigned char" : "char";
    }
    else if (type->GetSize() == 4) {
      return type->IsUnsigned() ? "unsigned int" : "int";
    }
  }
  else if (type->IsStruct()) {
    // treat structures as bytes
    return DefineStruct(type);
  }
  else if (type->IsArray()) {
    // arrays
    return DefineArray(type);
  }
  else if (type->IsPointer()) {
    // pointers
    return GetTypeName(type->GetDerefedType()) + "*";
  }
  assert(false);
  return "";
}

const std::string &ARMCodeGen::DefineStruct(const TypePtr &type) {
  assert(type->IsStruct());
  // try to find in defined types
  for (const auto &i : defed_types_) {
    if (i.first->IsIdentical(type)) return i.second;
  }
  // log new definition
  auto name = type->GetTypeId() + std::to_string(counter_++);
  defed_types_.push_back({type, name});
  // generate type definition
  type_ << "typedef unsigned char " << name;
  type_ << '[' << type->GetSize() << "];" << std::endl;
  return defed_types_.back().second;
}

const std::string &ARMCodeGen::DefineArray(const TypePtr &type) {
  assert(type->IsArray());
  // try to find in defined types
  for (const auto &i : defed_types_) {
    if (i.first->IsIdentical(type)) return i.second;
  }
  // log new definition
  auto base_name = GetTypeName(type->GetDerefedType());
  std::ostringstream oss;
  for (const auto &c : base_name) oss << (std::isspace(c) ? '_' : c);
  oss << '_' << type->GetLength();
  defed_types_.push_back({type, oss.str()});
  // generate type definition
  type_ << "typedef " << base_name << ' ' << oss.str();
  type_ << '[' << type->GetLength() << "];" << std::endl;
  return defed_types_.back().second;
}

Reg ARMCodeGen::GetVal(const SSAPtr &ssa) {
  // get pointer of value
  last_int_ = nullptr;
  last_str_ = nullptr;
  last_arg_ref_ = nullptr;
  last_zero_ = nullptr;
  ignore_gene_ssa_ = true;
  ssa->GenerateCode(*this);
  ignore_gene_ssa_ = false;

  // handle each case
  if (last_int_) {
    // store number to temporary register
    asm_gen_.PushLoadImm(Reg::r8, last_int_->value());
    return Reg::r8;
  }
  else if (last_arg_ref_) {
    switch (last_arg_ref_->index()) {
      case 0: return Reg::r0;
      case 1: return Reg::r1;
      case 2: return Reg::r2;
      case 3: return Reg::r3;
      default: {
        auto offset = last_arg_ref_->index() * 4;
        asm_gen_.PushAsm(Opcode::LDR, Reg::r8, Reg::fp, -offset);
        return Reg::r8;
      }
    }
  }
  else if (last_zero_) {
    return Reg::r8;
  }
  else {
    // if (entry_->vars.find(ssa) != entry_->vars.end()) {
    //   // load global variable
    //   auto name = GetVarName(last_var_->id());
    //   asm_gen_.PushAsm(Opcode::LUI, Reg::r9, "%hi(" + name + ")");
    //   asm_gen_.PushAsm(Opcode::LW, Reg::r8, Reg::r9, "%lo(" + name +
    //   ")"); return Reg::r8;
    // }
    // else {
      // get position of variable
      auto pos = var_alloc_.GetPosition(ssa);
      if (auto slot = std::get_if<std::size_t>(&pos)) {
        // load from local area
        auto offset = var_alloc_.arg_area_size() + *slot * 4;
        asm_gen_.PushAsm(Opcode::LDR, Reg::r8, Reg::fp, offset);
        return Reg::r8;
      }
      else {
        // just return
        return *std::get_if<Reg>(&pos);
      }
    // }
  }
  
  return Reg::r8;
}

void ARMCodeGen::SetVal(Value &ssa, const Reg value) {
  // get pointer of value
  ignore_gene_ssa_ = true;
  ssa.GenerateCode(*this);
  ignore_gene_ssa_ = false;
  // handle each case
  // if (entry_->vars.find(ssa) != entry_->vars.end()) {
  //   // store to global variable
  //   auto name = GetVarName(last_var_->id());
  //   asm_gen_.PushAsm(Opcode::LDR, Reg::r9, name);
  //   asm_gen_.PushAsm(Opcode::STR, Reg::r9, value);
  // }
  // else {
    // get position of variable
    auto pos = var_alloc_.GetPosition(current_ssaptr);
    if (auto slot = std::get_if<std::size_t>(&pos)) {
      // store to local area
      auto offset = var_alloc_.arg_area_size() + *slot * 4;
      asm_gen_.PushAsm(Opcode::STR, value, Reg::fp, offset);
    }
    else {
      // just move
      auto dest = *std::get_if<Reg>(&pos);
      asm_gen_.PushMove(dest, value);
    }
  // }
}

void ARMCodeGen::Reset() {
  has_dump_global_var_ = false;
  counter_ = 0;
  defed_types_.clear();
  type_.str("");
  type_.clear();
  code_.str("");
  code_.clear();
  arr_depth_ = 0;
}

void ARMCodeGen::GenerateOn(LoadSSA &ssa) {
  if (ignore_gene_ssa_) return;
  // get address
  auto addr = GetVal(ssa[0].value());
  // generate load
  asm_gen_.PushAsm(Opcode::LDR, Reg::r8, addr, 0);
  // store to dest
  SetVal(ssa, Reg::r8);
}
void ARMCodeGen::GenerateOn(StoreSSA &ssa) {
  if (ignore_gene_ssa_) return;
  auto value = GetVal(ssa[0].value());
  auto addr = GetVal(ssa[1].value());
  // generate load
  asm_gen_.PushAsm(Opcode::STR, value, addr, 0);
}


void ARMCodeGen::GenerateOn(BinarySSA &ssa) {
  if (ignore_gene_ssa_) return;
  auto lhs = GetVal(ssa[0].value());
  if (lhs == Reg::r8) {
    asm_gen_.PushMove(Reg::r9, lhs);
    lhs = Reg::r9;
  }
  auto rhs = GetVal(ssa[1].value());

  switch (ssa.op()) {
    // Add, Sub, Mul, UDiv, SDiv, URem, SRem, Equal, NotEq,
    // ULess, SLess, ULessEq, SLessEq, UGreat, SGreat, UGreatEq, SGreatEq,
    // And, Or, Xor, Shl, LShr, AShr,
    case BinarySSA::Operator::Add: {
      asm_gen_.PushAsm(Opcode::ADD, Reg::r8, lhs, rhs);
      break;
    }
    case BinarySSA::Operator::Sub: {
      asm_gen_.PushAsm(Opcode::SUB, Reg::r8, lhs, Reg::r9);
      break;
    }
    case BinarySSA::Operator::Mul: {
      asm_gen_.PushAsm(Opcode::MUL, Reg::r10, lhs, Reg::r9);
      asm_gen_.PushMove(Reg::r8, Reg::r10);
      break;
    }
    case BinarySSA::Operator::SDiv: {
      asm_gen_.PushAsm("stmfd\tsp!, {r0-r7}");
      asm_gen_.PushMove(Reg::r0, lhs);
      asm_gen_.PushMove(Reg::r1, rhs);
      asm_gen_.PushAsm(Opcode::BL, kDiv);
      asm_gen_.PushMove(Reg::r8, Reg::r0);
      asm_gen_.PushAsm("ldmfd\tsp!, {r0-r7}");
      break;
    }
    case BinarySSA::Operator::UDiv: {
      asm_gen_.PushAsm("stmfd\tsp!, {r0-r7}");
      asm_gen_.PushMove(Reg::r0, lhs);
      asm_gen_.PushMove(Reg::r1, rhs);
      asm_gen_.PushAsm(Opcode::BL, kUDiv);
      asm_gen_.PushMove(Reg::r8, Reg::r0);
      asm_gen_.PushAsm("ldmfd\tsp!, {r0-r7}");
      break;
    }
    case BinarySSA::Operator::SRem: {
      asm_gen_.PushAsm("stmfd\tsp!, {r0-r7}");
      asm_gen_.PushMove(Reg::r0, lhs);
      asm_gen_.PushMove(Reg::r1, rhs);
      asm_gen_.PushAsm(Opcode::BL, kRem);
      asm_gen_.PushMove(Reg::r8, Reg::r0);
      asm_gen_.PushAsm("ldmfd\tsp!, {r0-r7}");
      break;
    }
    case BinarySSA::Operator::URem: {
      asm_gen_.PushAsm("stmfd\tsp!, {r0-r7}");
      asm_gen_.PushMove(Reg::r0, lhs);
      asm_gen_.PushMove(Reg::r1, rhs);
      asm_gen_.PushAsm(Opcode::BL, kURem);
      asm_gen_.PushMove(Reg::r8, Reg::r0);
      asm_gen_.PushAsm("ldmfd\tsp!, {r0-r7}");
      break;
    }
    case BinarySSA::Operator::SLess: {
      asm_gen_.PushAsm(Opcode::CMP, lhs, rhs);
      asm_gen_.PushAsm(Opcode::MOVLT, Reg::r8, 1);
      asm_gen_.PushAsm(Opcode::MOVGE, Reg::r8, 0);
      break;
    }
    case BinarySSA::Operator::ULess: {
      asm_gen_.PushAsm(Opcode::CMP, lhs, rhs);
      asm_gen_.PushAsm(Opcode::MOVLO, Reg::r8, 1);
      asm_gen_.PushAsm(Opcode::MOVHS, Reg::r8, 0);
      break;
    }
    case BinarySSA::Operator::SLessEq: {
      asm_gen_.PushAsm(Opcode::CMP, lhs, rhs);
      asm_gen_.PushAsm(Opcode::MOVLT, Reg::r8, 1);
      asm_gen_.PushAsm(Opcode::MOVGE, Reg::r8, 0);
      break;
    }
    case BinarySSA::Operator::ULessEq: {
      asm_gen_.PushAsm(Opcode::CMP, lhs, rhs);
      asm_gen_.PushAsm(Opcode::MOVLS, Reg::r8, 1);
      asm_gen_.PushAsm(Opcode::MOVHI, Reg::r8, 0);
      break;
    }
    case BinarySSA::Operator::SGreat: {
      asm_gen_.PushAsm(Opcode::CMP, lhs, rhs);
      asm_gen_.PushAsm(Opcode::MOVGT, Reg::r8, 1);
      asm_gen_.PushAsm(Opcode::MOVLE, Reg::r8, 0);
      break;
    }
    case BinarySSA::Operator::UGreat: {
      asm_gen_.PushAsm(Opcode::CMP, lhs, rhs);
      asm_gen_.PushAsm(Opcode::MOVHI, Reg::r8, 1);
      asm_gen_.PushAsm(Opcode::MOVLS, Reg::r8, 0);
      break;
    }
    case BinarySSA::Operator::SGreatEq: {
      asm_gen_.PushAsm(Opcode::CMP, lhs, rhs);
      asm_gen_.PushAsm(Opcode::MOVGE, Reg::r8, 1);
      asm_gen_.PushAsm(Opcode::MOVLT, Reg::r8, 0);
      break;
    }
    case BinarySSA::Operator::UGreatEq: {
      asm_gen_.PushAsm(Opcode::CMP, lhs, rhs);
      asm_gen_.PushAsm(Opcode::MOVHS, Reg::r8, 1);
      asm_gen_.PushAsm(Opcode::MOVLO, Reg::r8, 0);
      break;
    }
    case BinarySSA::Operator::Equal: {
      asm_gen_.PushAsm(Opcode::CMP, lhs, rhs);
      asm_gen_.PushAsm(Opcode::MOVEQ, Reg::r8, 1);
      asm_gen_.PushAsm(Opcode::MOVNE, Reg::r8, 0);
      break;
    }
    case BinarySSA::Operator::NotEq: {
      asm_gen_.PushAsm(Opcode::CMP, lhs, rhs);
      asm_gen_.PushAsm(Opcode::MOVNE, Reg::r8, 1);
      asm_gen_.PushAsm(Opcode::MOVEQ, Reg::r8, 0);
      break;
    }
    case BinarySSA::Operator::And: {
      asm_gen_.PushAsm(Opcode::AND, Reg::r8, lhs, rhs);
    }
    case BinarySSA::Operator::Or: {
      asm_gen_.PushAsm(Opcode::ORR, Reg::r8, lhs, rhs);
    }
    case BinarySSA::Operator::Xor: {
      asm_gen_.PushAsm(Opcode::EOR, Reg::r8, lhs, rhs);
    }
    case BinarySSA::Operator::Shl: {
      asm_gen_.PushAsm(Opcode::LSL, Reg::r8, lhs, rhs);
    }
    case BinarySSA::Operator::LShr: {
      asm_gen_.PushAsm(Opcode::LSR, Reg::r8, lhs, rhs);
    }
    case BinarySSA::Operator::AShr: {
      asm_gen_.PushAsm(Opcode::ASR, Reg::r8, lhs, rhs);
    }
    default:
      // assert(false);
      std::cout << "error op: " << std::endl;
  }
  SetVal(ssa, Reg::r8);
}

void ARMCodeGen::GenerateOn(UnarySSA &ssa) {
  code_ << "UnarySSA" << std::endl;
  if (ignore_gene_ssa_) return;
  auto lhs = GetVal(ssa[0].value());

  switch (ssa.op()) {
    case UnarySSA::Operator::Neg: {
      asm_gen_.PushAsm(Opcode::MVN, Reg::r8, lhs);
      asm_gen_.PushAsm(Opcode::ADD, Reg::r8, Reg::r8, 1);
    }
    case UnarySSA::Operator::LogicNot: {
      asm_gen_.PushAsm(Opcode::CMP, lhs, 0);
      asm_gen_.PushAsm(Opcode::MOVEQ, Reg::r9, 0);
      asm_gen_.PushAsm(Opcode::MOVNE, Reg::r9, 1);
      asm_gen_.PushAsm(Opcode::MOV, Reg::r8, Reg::r9);
    }
    case UnarySSA::Operator::Not: {
      asm_gen_.PushAsm(Opcode::MVN, Reg::r8, lhs);
    }
  }
  SetVal(ssa, Reg::r8);
}

void ARMCodeGen::GenerateOn(AccessSSA &ssa) {
  code_ << "AccessSSA" << std::endl;
}

void ARMCodeGen::GenerateOn(CastSSA &ssa) {
  code_ << "CastSSA" << std::endl;
   
}

void ARMCodeGen::GenerateOn(CallSSA &ssa) {
  code_ << "CallSSA" << std::endl;
  
}
void ARMCodeGen::GenerateOn(BranchSSA &ssa) {
  auto cond = GetVal(ssa[0].value());
  asm_gen_.PushAsm(Opcode::CMP, cond, 0);
  asm_gen_.PushAsm(Opcode::BEQ, GetLabel(ssa[1].value()));
  asm_gen_.PushAsm(Opcode::BNE, GetLabel(ssa[2].value()));
}

void ARMCodeGen::GenerateOn(JumpSSA &ssa) {
  asm_gen_.PushJump(GetLabel(ssa[0].value()));
}

void ARMCodeGen::GenerateOn(ReturnSSA &ssa) {
  auto value = GetVal(ssa[0].value());
  asm_gen_.PushMove(Reg::r0, value);
  // generate epilogue
  asm_gen_.PushAsm(Opcode::INST, "ldmea\tfp, {r0-r7,fp,sp,pc}");
}

void ARMCodeGen::GenerateOn(FunctionSSA &ssa) {
  var_alloc_.RunOnFunction(ssa);
  // generate function declaration
  const auto &type = ssa.type();
  auto args_ty = *type->GetArgsType();
  auto ret_ty = type->GetReturnType(args_ty);
  code_ << GetLinkType(ssa.link());
  code_ << ssa.name() << '(';
  // generate arguments
  for (std::size_t i = 0; i < args_ty.size(); ++i) {
    if (i) code_ << ", ";
    code_ << GetTypeName(args_ty[i]);
  }
  code_ << "):\n";
  if (ssa.empty()) {
    // GenEnd(ssa);
    return;
  }
  // generate prologue
  asm_gen_.PushMove(Reg::ip, Reg:: sp);
  asm_gen_.PushAsm("stmfd\tsp!, {r0-r7,fp,ip,lr,pc}");
  // asm_gen_.PushAsm("sub\tfp, ip, #4");
  // std::size_t frame_sz = var_alloc_.local_area_size();
  // std::string inst = "sub\tfp, fp, #" + std::to_string(frame_sz);
  // asm_gen_.PushAsm(inst);

  // generate function body
  for (const auto i : ssa) {
    current_ssaptr = i.value();
    i.value()->GenerateCode(*this);
  }
  asm_gen_.Dump(code_, kIndent);
}
void ARMCodeGen::GenerateOn(GlobalVarSSA &ssa) {
  if (!has_dump_global_var_) {
    code_ << GetLinkType(ssa.link());
    // if (!ssa.is_var()) code_ << "const ";
    // generate type
    auto type = ssa.type()->GetDerefedType();
    code_ << ssa.name() << ":" << std::endl;
    code_ << kIndent << "." << GetTypeName(type) << std::endl;
    // generate initializer
    if (ssa[0].value()) {
      in_global_var_ = true;
      code_ << kIndent
            << std::any_cast<std::string>(&ssa[0].value()->metadata())
            << std::endl;
      in_global_var_ = false;
    }
    has_dump_global_var_ = true;
  }
  else if (!ignore_gene_ssa_) {

  }
}

void ARMCodeGen::GenerateOn(BlockSSA &ssa) {
  std::string label;
  if (ssa.metadata().has_value()) {
    label = *std::any_cast<std::string>(&ssa.metadata());
  }
  else {
    label = GetNewVar(kLabelPrefix);
    ssa.set_metadata(label);
  }
  asm_gen_.PushLabel(label);
  for (const auto i : ssa.insts()) {
    current_ssaptr = i;
    i->GenerateCode(*this);
  }
}

void ARMCodeGen::GenerateOn(ConstStructSSA &ssa) {
  code_ << "ConstStructSSA " << std::endl;
}

void ARMCodeGen::GenerateOn(ConstArraySSA &ssa) {
  code_ << "ConstArraySSA " << std::endl;
}

void ARMCodeGen::GenerateOn(AllocaSSA &ssa) {
  // code_ << "GenerateOn AllocaSSA" << std::endl;
}

void ARMCodeGen::GenerateOn(ArgRefSSA &ssa) { last_arg_ref_ = &ssa; }
void ARMCodeGen::GenerateOn(ConstIntSSA &ssa) { last_int_ = &ssa; }
void ARMCodeGen::GenerateOn(ConstStrSSA &ssa) { last_str_ = &ssa; }
void ARMCodeGen::GenerateOn(ConstZeroSSA &ssa) { last_zero_ = &ssa; }

void ARMCodeGen::GenerateOn(SelectSSA &ssa) {}
void ARMCodeGen::GenerateOn(UndefSSA &ssa) {}

void ARMCodeGen::Dump(std::ostream &os) const {
  os << type_.str() << std::endl;
  os << code_.str() << std::endl;
}
