#ifndef BACK_ASM_MIR_MIR_H_
#define BACK_ASM_MIR_MIR_H_

#include <ostream>
#include <memory>
#include <vector>
#include <list>

namespace mimic::back::asmgen {

// base class of all operands
class OperandBase {
 public:
  virtual ~OperandBase() = default;

  // check if operand is a register
  virtual bool IsReg() const = 0;
  // check if operand is a virtual register
  virtual bool IsVirtual() const = 0;
  // check if operand is a immediate value
  virtual bool IsImm() const = 0;
  // check if operand is a label
  virtual bool IsLabel() const = 0;
  // check if operand is a stack slot
  virtual bool IsSlot() const = 0;

  // dump operand to output stream
  virtual void Dump(std::ostream &os) const = 0;
};

// pointer to operand
using OprPtr = std::shared_ptr<OperandBase>;
using OprPtrList = std::vector<OprPtr>;

// base class of all instruction (machine IR)
class InstBase {
 public:
  virtual ~InstBase() = default;

  // check if is a move instruction (for general purpose optimizations)
  virtual bool IsMove() const = 0;
  // dump instruction to output stream
  virtual void Dump(std::ostream &os) const = 0;

  // add an operand to current instruction
  void AddOpr(const OprPtr &opr) { oprs_.push_back(opr); }

  // setters
  void set_dest(const OprPtr &dest) { dest_ = dest; }

  // getters
  const OprPtr &dest() const { return dest_; }
  const OprPtrList &oprs() const { return oprs_; }

 private:
  OprPtr dest_;
  OprPtrList oprs_;
};

// pointer to instruction
using InstPtr = std::shared_ptr<InstBase>;
using InstPtrList = std::list<InstPtr>;

}  // namespace mimic::back::asmgen

#endif  // BACK_ASM_MIR_MIR_H_
