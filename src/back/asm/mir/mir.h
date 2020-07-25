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
  OperandBase() : use_count_(0) {}
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

  // getters
  std::size_t use_count() const { return use_count_; }

 private:
  friend class Use;
  std::size_t use_count_;
};

// pointer to operand
using OprPtr = std::shared_ptr<OperandBase>;

// a LLVM-like use structure for using operands
// but only count number of users
class Use {
 public:
  explicit Use(const OprPtr &value) : value_(value) {
    if (value_) ++value_->use_count_;
  }
  // copy constructor
  Use(const Use &use) : value_(use.value_) {
    if (value_) ++value_->use_count_;
  }
  // move constructor
  Use(Use &&use) noexcept : value_(std::move(use.value_)) {}
  // destructor
  ~Use() {
    if (value_) --value_->use_count_;
  }

  // copy assignment operator
  Use &operator=(const Use &use) {
    // update reference
    if (this != &use) set_value(use.value_);
    return *this;
  }

  // move assignment operator
  Use &operator=(Use &&use) noexcept {
    if (this != &use) {
      // update reference
      if (use.value_) --use.value_->use_count_;
      set_value(std::move(use.value_));
    }
    return *this;
  }

  // setters
  void set_value(const OprPtr &value) {
    if (value != value_) {
      if (value_) --value_->use_count_;
      value_ = value;
      if (value_) ++value_->use_count_;
    }
  }

  // getters
  const OprPtr &value() const { return value_; }

 private:
  OprPtr value_;
};

// base class of all instruction (machine IR)
class InstBase {
 public:
  virtual ~InstBase() = default;

  // check if is a move instruction (for general purpose optimizations)
  virtual bool IsMove() const = 0;
  // dump instruction to output stream
  virtual void Dump(std::ostream &os) const = 0;

  // add an operand to current instruction
  void AddOpr(const OprPtr &opr) { oprs_.push_back(Use(opr)); }

  // setters
  void set_dest(const OprPtr &dest) { dest_ = dest; }

  // getters
  const OprPtr &dest() const { return dest_; }
  std::vector<Use> &oprs() { return oprs_; }
  const std::vector<Use> &oprs() const { return oprs_; }

 private:
  OprPtr dest_;
  std::vector<Use> oprs_;
};

// pointer to instruction
using InstPtr = std::shared_ptr<InstBase>;
using InstPtrList = std::list<InstPtr>;

}  // namespace mimic::back::asmgen

#endif  // BACK_ASM_MIR_MIR_H_
