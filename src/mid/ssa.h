#ifndef MIMIC_MID_SSA_H_
#define MIMIC_MID_SSA_H_

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

#include "mid/usedef.h"

// declare a getter/setter method of SSA
#define DECL_GETTER_SETTER(name, idx)                         \
  const SSAPtr &name() const { return (*this)[idx].value(); } \
  void set_##name(const SSAPtr &name) {                       \
    return (*this)[idx].set_value(name);                      \
  }

namespace mimic::mid {

// forward declarations
class BlockSSA;
class FunctionSSA;
class GlobalVarSSA;

// type aliases
using BlockPtr = std::shared_ptr<BlockSSA>;
using FuncPtr = std::shared_ptr<FunctionSSA>;
using GlobalVarPtr = std::shared_ptr<GlobalVarSSA>;

// linkage types
enum class LinkageTypes {
  Internal, Inline, External, GlobalCtor, GlobalDtor,
};

// load from allocation
// operands: pointer
class LoadSSA : public User {
 public:
  LoadSSA(const SSAPtr &ptr) : addr_(ptr) {
    Reserve(1);
    AddValue(ptr);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  SSAPtr GetAddr() const override { return addr_.lock(); }
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return ptr()->IsUndef(); }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(ptr, 0);

 private:
  SSARef addr_;
};

// store to allocation
// operands: value, pointer
class StoreSSA : public User {
 public:
  StoreSSA(const SSAPtr &value, const SSAPtr &ptr) {
    Reserve(2);
    AddValue(value);
    AddValue(ptr);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(value, 0);
  DECL_GETTER_SETTER(ptr, 1);
};

// element accessing (load effective address)
// operands: ptr, index
class AccessSSA : public User {
 public:
  enum class AccessType { Pointer, Element };

  AccessSSA(AccessType acc_type, const SSAPtr &ptr, const SSAPtr &index)
      : acc_type_(acc_type) {
    Reserve(2);
    AddValue(ptr);
    AddValue(index);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override {
    return ptr()->IsUndef() || index()->IsUndef();
  }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(ptr, 0);
  DECL_GETTER_SETTER(index, 1);

  // getters
  AccessType acc_type() const { return acc_type_; }

 private:
  AccessType acc_type_;
};

// binary operations
// operands: lhs, rhs
class BinarySSA : public User {
 public:
  enum class Operator {
    Add, Sub, Mul, UDiv, SDiv, URem, SRem, Equal, NotEq,
    ULess, SLess, ULessEq, SLessEq, UGreat, SGreat, UGreatEq, SGreatEq,
    And, Or, Xor, Shl, LShr, AShr,
  };

  BinarySSA(Operator op, const SSAPtr &lhs, const SSAPtr &rhs) : op_(op) {
    Reserve(2);
    AddValue(lhs);
    AddValue(rhs);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override {
    return lhs()->IsUndef() || rhs()->IsUndef();
  }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // check if is a comparison instruction
  bool IsCmp() const {
    return static_cast<int>(op_) >= static_cast<int>(Operator::Equal) &&
           static_cast<int>(op_) <= static_cast<int>(Operator::SGreatEq);
  }

  // getter/setter
  DECL_GETTER_SETTER(lhs, 0);
  DECL_GETTER_SETTER(rhs, 1);

  // setters
  void set_op(Operator op) { op_ = op; }

  // getters
  Operator op() const { return op_; }

 private:
  Operator op_;
};

// unary operations
// operands: opr
class UnarySSA : public User {
 public:
  enum class Operator {
    Neg, LogicNot, Not,
  };

  UnarySSA(Operator op, const SSAPtr &opr) : op_(op) {
    Reserve(1);
    AddValue(opr);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return opr()->IsUndef(); }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(opr, 0);

  // getters
  Operator op() const { return op_; }

 private:
  Operator op_;
};

// type casting
// operands: opr
class CastSSA : public User {
 public:
  CastSSA(const SSAPtr &opr) {
    Reserve(1);
    AddValue(opr);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return opr()->IsConst(); }
  bool IsUndef() const override { return opr()->IsUndef(); }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(opr, 0);
};

// function call
// operands: callee, arg1, arg2, ...
class CallSSA : public User {
 public:
  CallSSA(const SSAPtr &callee, const SSAPtrList &args) {
    Reserve(args.size() + 1);
    AddValue(callee);
    for (const auto &i : args) AddValue(i);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return callee()->IsUndef(); }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(callee, 0);
};

// conditional branch
// operands: cond, true, false
class BranchSSA : public User {
 public:
  BranchSSA(const SSAPtr &cond, const SSAPtr &true_block,
            const SSAPtr &false_block) {
    Reserve(3);
    AddValue(cond);
    AddValue(true_block);
    AddValue(false_block);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(cond, 0);
  DECL_GETTER_SETTER(true_block, 1);
  DECL_GETTER_SETTER(false_block, 2);
};

// unconditional jump
// operands: target
class JumpSSA : public User {
 public:
  JumpSSA(const SSAPtr &target) {
    Reserve(1);
    AddValue(target);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(target, 0);
};

// function return
// operands: value
// NOTE: value can be 'nullptr'
class ReturnSSA : public User {
 public:
  ReturnSSA(const SSAPtr &value) {
    Reserve(1);
    AddValue(value);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(value, 0);
};

// function definition/declaration
// operands: bb1 (entry), bb2, ...
class FunctionSSA : public User {
 public:
  FunctionSSA(LinkageTypes link, const std::string &name)
      : link_(link), name_(name) {}

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(entry, 0);

  // setters
  void set_link(LinkageTypes link) { link_ = link; }
  void set_arg(std::size_t i, const SSAPtr &arg) {
    args_.resize(i + 1);
    args_[i] = arg;
  }

  // getters
  LinkageTypes link() const { return link_; }
  const std::string &name() const { return name_; }
  bool is_decl() const { return empty(); }
  const std::vector<SSAPtr> &args() const { return args_; }

 private:
  LinkageTypes link_;
  std::string name_;
  std::vector<SSAPtr> args_;
};

// global variable definition/declaration
// operands: initializer
class GlobalVarSSA : public User {
 public:
  GlobalVarSSA(LinkageTypes link, bool is_var, const std::string &name,
               const SSAPtr &init)
      : link_(link), is_var_(is_var), name_(name) {
    Reserve(1);
    AddValue(init);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(init, 0);

  // setters
  void set_link(LinkageTypes link) { link_ = link; }
  void set_is_var(bool is_var) { is_var_ = is_var; }

  // getters
  LinkageTypes link() const { return link_; }
  bool is_var() const { return is_var_; }
  const std::string &name() const { return name_; }

 private:
  LinkageTypes link_;
  bool is_var_;
  std::string name_;
};

// memory allocation
class AllocaSSA : public Value {
 public:
  AllocaSSA() {}

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;
};

// basic block
// operands: pred1, pred2, ...
class BlockSSA : public User {
 public:
  BlockSSA(const UserPtr &parent, const std::string &name)
      : name_(name), parent_(parent) {}

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // add a new instruction
  void AddInst(const SSAPtr &inst) { insts_.push_back(inst); }

  // setters
  void set_parent(const UserPtr &parent) { parent_ = parent; }

  // getters
  const std::string &name() const { return name_; }
  const UserPtr &parent() const { return parent_; }
  SSAPtrList &insts() { return insts_; }

 private:
  // block name
  std::string name_;
  // parent function
  UserPtr parent_;
  // instructions in current block
  SSAPtrList insts_;
};

// argument reference
class ArgRefSSA : public Value {
 public:
  ArgRefSSA(const SSAPtr &func, std::size_t index)
      : func_(func), index_(index) {}

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getters
  const SSAPtr &func() const { return func_; }
  std::size_t index() const { return index_; }

 private:
  SSAPtr func_;
  std::size_t index_;
};

// constant integer
class ConstIntSSA : public Value {
 public:
  ConstIntSSA(std::uint32_t value) : value_(value) {}

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return true; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getters
  std::uint32_t value() const { return value_; }

 private:
  std::uint32_t value_;
};

// constant string
class ConstStrSSA : public Value {
 public:
  ConstStrSSA(const std::string &str) : str_(str) {}

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return true; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getters
  const std::string &str() const { return str_; }

 private:
  std::string str_;
};

// constant structure
// operands: elem1, elem2, ...
class ConstStructSSA : public User {
 public:
  ConstStructSSA(const SSAPtrList &elems) {
    Reserve(elems.size());
    for (const auto &i : elems) AddValue(i);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return true; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;
};

// constant array
// operands: elem1, elem2, ...
class ConstArraySSA : public User {
 public:
  ConstArraySSA(const SSAPtrList &elems) {
    Reserve(elems.size());
    for (const auto &i : elems) AddValue(i);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return true; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;
};

// constant zero
class ConstZeroSSA : public Value {
 public:
  ConstZeroSSA() {}

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return true; }
  bool IsUndef() const override { return false; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;
};

// operand of phi node
// operands: value, block (which value comes from)
class PhiOperandSSA : public User {
 public:
  PhiOperandSSA(const SSAPtr &val, const SSAPtr &block) {
    Reserve(2);
    AddValue(val);
    AddValue(block);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  // defined in 'ssa.cpp'
  bool IsUndef() const override;

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(value, 0);
  DECL_GETTER_SETTER(block, 1);
};

// phi node
// operands: phiopr1, phiopr2, ...
class PhiSSA : public User {
 public:
  PhiSSA(const SSAPtrList &oprs) {
    Reserve(oprs.size());
    for (const auto &i : oprs) { AddValue(i); }
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override {
    for (const auto &i : *this) {
      if (!i.value()->IsUndef()) return false;
    }
    return true;
  }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;
};

// select instruction
// operands: cond, true, false
class SelectSSA : public User {
 public:
  SelectSSA(const SSAPtr &cond, const SSAPtr &true_val,
            const SSAPtr &false_val) {
    Reserve(3);
    AddValue(cond);
    AddValue(true_val);
    AddValue(false_val);
  }

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override {
    return cond()->IsUndef() ||
           (true_val()->IsUndef() && false_val()->IsUndef());
  }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;

  // getter/setter
  DECL_GETTER_SETTER(cond, 0);
  DECL_GETTER_SETTER(true_val, 1);
  DECL_GETTER_SETTER(false_val, 2);
};

// undefined value
class UndefSSA : public Value {
 public:
  UndefSSA() {}

  void Dump(std::ostream &os, IdManager &idm) const override;
  bool IsConst() const override { return false; }
  bool IsUndef() const override { return true; }

  void RunPass(opt::PassBase &pass) override;
  void GenerateCode(back::CodeGen &gen) override;
};

}  // namespace mimic::mid

#undef DECL_GETTER_SETTER

#endif  // MIMIC_MID_SSA_H_
