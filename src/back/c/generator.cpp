#include "back/c/generator.h"

#include <cassert>

#include "utils/strprint.h"

using namespace mimic::define;
using namespace mimic::mid;
using namespace mimic::back::c;

namespace {

const char *kVarPrefix = "v", *kLabelPrefix = "b", *kArgPrefix = "a";
const char *kIndent = "  ";

// stringfy binary operator of 'BinarySSA'
inline std::string_view GetBinOp(BinarySSA::Operator op) {
  using Op = BinarySSA::Operator;
  switch (op) {
    case Op::Add: return "+";
    case Op::Sub: return "-";
    case Op::Mul: return "*";
    case Op::UDiv: case Op::SDiv: return "/";
    case Op::URem: case Op::SRem: return "%";
    case Op::Equal: return "==";
    case Op::NotEq: return "!=";
    case Op::ULess: case Op::SLess: return "<";
    case Op::ULessEq: case Op::SLessEq: return "<=";
    case Op::UGreat: case Op::SGreat: return ">";
    case Op::UGreatEq: case Op::SGreatEq: return ">=";
    case Op::And: return "&";
    case Op::Or: return "|";
    case Op::Xor: return "^";
    case Op::Shl: return "<<";
    case Op::LShr: case Op::AShr: return ">>";
    default: assert(false); return "";
  }
}

// stringfy unary operator of 'UnarySSA'
inline std::string_view GetUnaOp(UnarySSA::Operator op) {
  using Op = UnarySSA::Operator;
  switch (op) {
    case Op::Neg: return "-";
    case Op::LogicNot: return "!";
    case Op::Not: return "~";
    default: assert(false); return "";
  }
}

inline std::string_view GetLinkType(LinkageTypes link) {
  switch (link) {
    case LinkageTypes::Internal: return "static ";
    case LinkageTypes::Inline: return "inline ";
    case LinkageTypes::GlobalCtor: return "__attribute((constructor)) ";
    case LinkageTypes::GlobalDtor: return "__attribute((destructor)) ";
    default: return "";
  }
}

}  // namespace

const std::string &CCodeGen::GetVal(const SSAPtr &ssa) {
  auto val = std::any_cast<std::string>(&ssa->metadata());
  if (!val) {
    ssa->GenerateCode(*this);
    val = std::any_cast<std::string>(&ssa->metadata());
    assert(val);
  }
  return *val;
}

const std::string &CCodeGen::GetLabel(const mid::SSAPtr &ssa) {
  auto val = std::any_cast<std::string>(&ssa->metadata());
  if (!val) {
    ssa->set_metadata(GetNewVar(kLabelPrefix));
    val = std::any_cast<std::string>(&ssa->metadata());
  }
  return *val;
}

void CCodeGen::SetVal(Value &ssa, const std::string &val) {
  ssa.set_metadata(val);
}

std::string CCodeGen::GetNewVar(const char *prefix) {
  return prefix + std::to_string(counter_++);
}

std::string CCodeGen::DeclVar(Value &ssa) {
  assert(!ssa.type()->IsVoid());
  auto var = GetNewVar(kVarPrefix);
  code_ << kIndent << GetTypeName(ssa.type()) << ' ' << var << " = ";
  return var;
}

void CCodeGen::GenEnd(Value &ssa) {
  code_ << "; // ";
  code_ << ssa.logger()->line_pos() << ':' << ssa.logger()->col_pos();
  code_ << std::endl;
}

std::string CCodeGen::GetTypeName(const TypePtr &type) {
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

const std::string &CCodeGen::DefineStruct(const TypePtr &type) {
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

const std::string &CCodeGen::DefineArray(const TypePtr &type) {
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

void CCodeGen::Reset() {
  counter_ = 0;
  defed_types_.clear();
  type_.str("");
  type_.clear();
  global_alloca_.str("");
  global_alloca_.clear();
  code_.str("");
  code_.clear();
  in_global_var_ = false;
  arr_depth_ = 0;
}

void CCodeGen::GenerateOn(LoadSSA &ssa) {
  auto var = GetNewVar(kVarPrefix), val = GetVal(ssa.ptr());
  const auto &type = ssa.type();
  code_ << kIndent << GetTypeName(type) << ' ' << var;
  if (type->IsArray() || type->IsStruct()) {
    // generate memory copy
    code_ << ';' << std::endl;
    code_ << kIndent << "memcpy((void*)&" << var << ", (const void*)";
    code_ << val << ", " << type->GetSize() << ')';
  }
  else {
    // just assign
    code_ << " = *" << val;
  }
  GenEnd(ssa);
  SetVal(ssa, var);
}

void CCodeGen::GenerateOn(StoreSSA &ssa) {
  const auto &ptr = ssa.ptr(), &val = ssa.value();
  auto type = ptr->type()->GetDerefedType();
  auto ptr_val = GetVal(ptr), val_val = GetVal(val);
  if (type->IsArray() || type->IsStruct()) {
    // generate memory copy
    code_ << kIndent << "memcpy((void*)" << ptr_val << ", (const void*)&";
    code_ << val_val << ", " << type->GetSize() << ')';
  }
  else {
    // just assign
    code_ << "  *" << ptr_val << " = " << val_val;
  }
  GenEnd(ssa);
}

void CCodeGen::GenerateOn(AccessSSA &ssa) {
  using AccType = AccessSSA::AccessType;
  auto var = DeclVar(ssa);
  auto ptr = GetVal(ssa.ptr()), index = GetVal(ssa.index());
  // handle by access type
  if (ssa.acc_type() == AccType::Pointer) {
    // pointer
    code_ << ptr << " + " << index;
  }
  else {
    assert(ssa.acc_type() == AccType::Element);
    auto base_ty = ssa.ptr()->type()->GetDerefedType();
    code_ << '(' << GetTypeName(ssa.type()) << ')';
    if (base_ty->IsArray()) {
      // array
      code_ << "&(*" << ptr << ")[" << index << ']';
    }
    else {
      // structure
      assert(base_ty->IsStruct());
      code_ << "((unsigned char*)" << ptr << " + ";
      assert(ssa.index()->IsConst());
      // calculate offset
      std::size_t idx = std::stoul(index), offset = 0;
      auto align = base_ty->GetAlignSize();
      for (std::size_t i = 0; i < idx; ++i) {
        auto k = (base_ty->GetElem(i)->GetSize() + align - 1) / align;
        offset += align * k;
      }
      code_ << offset << ')';
    }
  }
  GenEnd(ssa);
  SetVal(ssa, var);
}

void CCodeGen::GenerateOn(BinarySSA &ssa) {
  auto var = DeclVar(ssa);
  code_ << GetVal(ssa.lhs()) << ' ' << GetBinOp(ssa.op());
  code_ << ' ' << GetVal(ssa.rhs());
  GenEnd(ssa);
  SetVal(ssa, var);
}

void CCodeGen::GenerateOn(UnarySSA &ssa) {
  auto var = DeclVar(ssa);
  code_ << GetUnaOp(ssa.op()) << GetVal(ssa.opr());
  GenEnd(ssa);
  SetVal(ssa, var);
}

void CCodeGen::GenerateOn(CastSSA &ssa) {
  auto val = '(' + GetTypeName(ssa.type()) + ')' + GetVal(ssa.opr());
  if (!ssa.IsConst()) {
    auto var = DeclVar(ssa);
    code_ << val;
    GenEnd(ssa);
    SetVal(ssa, var);
  }
  else {
    SetVal(ssa, val);
  }
}

void CCodeGen::GenerateOn(CallSSA &ssa) {
  if (!ssa.type()->IsVoid()) {
    auto var = DeclVar(ssa);
    SetVal(ssa, var);
  }
  else {
    code_ << kIndent;
  }
  code_ << GetVal(ssa.callee()) << '(';
  // generate arguments
  for (std::size_t i = 1; i < ssa.size(); ++i) {
    if (i > 1) code_ << ", ";
    code_ << GetVal(ssa[i].value());
  }
  code_ << ')';
  GenEnd(ssa);
}

void CCodeGen::GenerateOn(BranchSSA &ssa) {
  code_ << kIndent << "if (" << GetVal(ssa.cond()) << ") goto ";
  code_ << GetLabel(ssa.true_block()) << "; else goto ";
  code_ << GetLabel(ssa.false_block());
  GenEnd(ssa);
}

void CCodeGen::GenerateOn(JumpSSA &ssa) {
  code_ << kIndent << "goto ";
  code_ << GetLabel(ssa.target());
  GenEnd(ssa);
}

void CCodeGen::GenerateOn(ReturnSSA &ssa) {
  code_ << kIndent << "return";
  if (ssa.value()) code_ << ' ' << GetVal(ssa.value());
  GenEnd(ssa);
}

void CCodeGen::GenerateOn(FunctionSSA &ssa) {
  // generate function declaration
  const auto &type = ssa.type();
  auto args_ty = *type->GetArgsType();
  auto ret_ty = type->GetReturnType(args_ty);
  code_ << GetLinkType(ssa.link()) << GetTypeName(ret_ty) << ' ';
  code_ << ssa.name() << '(';
  SetVal(ssa, ssa.name());
  // generate arguments
  for (std::size_t i = 0; i < args_ty.size(); ++i) {
    if (i) code_ << ", ";
    code_ << GetTypeName(args_ty[i]) << ' ' << kArgPrefix << i;
  }
  code_ << ')';
  if (ssa.is_decl()) {
    GenEnd(ssa);
    return;
  }
  // generate function body
  code_ << " {" << std::endl;
  for (const auto &i : ssa) {
    i.value()->GenerateCode(*this);
  }
  code_ << '}' << std::endl;
}

void CCodeGen::GenerateOn(GlobalVarSSA &ssa) {
  code_ << GetLinkType(ssa.link());
  if (!ssa.is_var()) code_ << "const ";
  // generate type
  auto type = ssa.type()->GetDerefedType();
  code_ << GetTypeName(type) << ' ' << ssa.name();
  // generate initializer
  if (ssa.init()) {
    in_global_var_ = true;
    code_ << " = " << GetVal(ssa.init());
    in_global_var_ = false;
  }
  GenEnd(ssa);
  SetVal(ssa, '&' + ssa.name());
}

void CCodeGen::GenerateOn(AllocaSSA &ssa) {
  auto var = GetNewVar(kVarPrefix);
  if (ssa.type()->GetDerefedType()->GetSize() < 512) {
    auto type = ssa.type()->GetDerefedType();
    code_ << kIndent << GetTypeName(type) << ' ' << var;
    GenEnd(ssa);
  }
  else {
    // to large to put in function, generate as a global variable
    auto type = ssa.type()->GetDerefedType();
    global_alloca_ << GetTypeName(type) << ' ' << var;
    global_alloca_ << "; // ";
    global_alloca_ << ssa.logger()->line_pos() << ':';
    global_alloca_ << ssa.logger()->col_pos() << std::endl;
  }
  SetVal(ssa, '&' + var);
}

void CCodeGen::GenerateOn(BlockSSA &ssa) {
  // generate label
  std::string label;
  if (ssa.metadata().has_value()) {
    label = *std::any_cast<std::string>(&ssa.metadata());
  }
  else {
    label = GetNewVar(kLabelPrefix);
    SetVal(ssa, label);
  }
  code_ << label << ": (void)0;" << std::endl;
  // generate instructions
  for (const auto &i : ssa.insts()) {
    assert(!i->metadata().has_value());
    i->GenerateCode(*this);
  }
}

void CCodeGen::GenerateOn(ArgRefSSA &ssa) {
  SetVal(ssa, kArgPrefix + std::to_string(ssa.index()));
}

void CCodeGen::GenerateOn(ConstIntSSA &ssa) {
  if (ssa.type()->IsUnsigned() || ssa.type()->IsPointer()) {
    SetVal(ssa, std::to_string(ssa.value()));
  }
  else {
    SetVal(ssa, std::to_string(static_cast<std::int32_t>(ssa.value())));
  }
}

void CCodeGen::GenerateOn(ConstStrSSA &ssa) {
  std::ostringstream oss;
  oss << '"';
  utils::DumpStr(oss, ssa.str());
  oss << '"';
  SetVal(ssa, oss.str());
}

void CCodeGen::GenerateOn(ConstStructSSA &ssa) {
  // not implemented
  assert(false);
}

void CCodeGen::GenerateOn(ConstArraySSA &ssa) {
  ++arr_depth_;
  // generate elements
  std::ostringstream oss;
  oss << '{';
  for (std::size_t i = 0; i < ssa.size(); ++i) {
    if (i) oss << ", ";
    oss << GetVal(ssa[i].value());
  }
  oss << '}';
  // generate value
  if (in_global_var_ || (ssa.IsConst() && arr_depth_ > 1)) {
    SetVal(ssa, oss.str());
  }
  else {
    auto var = DeclVar(ssa);
    code_ << oss.str();
    GenEnd(ssa);
    SetVal(ssa, var);
  }
  --arr_depth_;
}

void CCodeGen::GenerateOn(ConstZeroSSA &ssa) {
  const auto &type = ssa.type();
  if (type->IsInteger()) {
    SetVal(ssa, "0");
  }
  else if (type->IsArray()) {
    // generate value
    if (in_global_var_ || arr_depth_) {
      SetVal(ssa, "{}");
    }
    else {
      auto var = DeclVar(ssa);
      code_ << "{}";
      GenEnd(ssa);
      SetVal(ssa, var);
    }
  }
  else {
    // not fully implemented
    assert(false);
  }
}

void CCodeGen::GenerateOn(SelectSSA &ssa) {
  auto var = DeclVar(ssa);
  code_ << GetVal(ssa.cond()) << " ? " << GetVal(ssa.true_val());
  code_ << " : " << GetVal(ssa.false_val());
  GenEnd(ssa);
  SetVal(ssa, var);
}

void CCodeGen::GenerateOn(UndefSSA &ssa) {
  // treat undefined value as zero
  SetVal(ssa, '(' + GetTypeName(ssa.type()) + ")0");
}

void CCodeGen::Dump(std::ostream &os) const {
  os << "#include <string.h>" << std::endl << std::endl;
  os << type_.str() << std::endl;
  os << global_alloca_.str() << std::endl;
  os << code_.str() << std::endl;
}
