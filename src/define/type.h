#ifndef MIMIC_DEFINE_TYPE_H_
#define MIMIC_DEFINE_TYPE_H_

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <optional>
#include <cstdint>
#include <cstddef>
#include <cassert>

namespace mimic::define {

// definition of base class of all types
class BaseType;
using TypePtr = std::shared_ptr<BaseType>;
using TypePtrList = std::vector<TypePtr>;
using TypePair = std::pair<std::string, TypePtr>;
using TypePairList = std::vector<TypePair>;

class BaseType {
 public:
  virtual ~BaseType() = default;

  // return true if is right value
  virtual bool IsRightValue() const = 0;
  // return true if is void type
  virtual bool IsVoid() const = 0;
  // return true if is integer
  virtual bool IsInteger() const = 0;
  // return true if is unsigned
  virtual bool IsUnsigned() const = 0;
  // return true if is structure type
  virtual bool IsStruct() const = 0;
  // return true if is constant type
  virtual bool IsConst() const = 0;
  // return true if is function type
  virtual bool IsFunction() const = 0;
  // return true if is array type
  virtual bool IsArray() const = 0;
  // return true if is pointer type
  virtual bool IsPointer() const = 0;
  // return true if left value which is current type
  // can accept the right value which is specific type
  virtual bool CanAccept(const TypePtr &type) const = 0;
  // return true if current type can be casted to specific type
  virtual bool CanCastTo(const TypePtr &type) const = 0;
  // return true if two types are identical
  // (ignore left/right value and const)
  virtual bool IsIdentical(const TypePtr &type) const = 0;
  // return the size of current type
  virtual std::size_t GetSize() const = 0;
  // return the alignment size of current type
  virtual std::size_t GetAlignSize() const = 0;
  // return the type of arguments of a function call
  virtual std::optional<TypePtrList> GetArgsType() const = 0;
  // return the return type of a function call
  virtual TypePtr GetReturnType(const TypePtrList &args) const = 0;
  // return the length of current type
  // e.g. array length, struct field number
  virtual std::size_t GetLength() const = 0;
  // return the element at specific index
  virtual TypePtr GetElem(std::size_t index) const = 0;
  // return the element with specific name
  virtual TypePtr GetElem(const std::string &name) const = 0;
  // return the index of element with specific name
  virtual std::optional<std::size_t> GetElemIndex(
      const std::string &name) const = 0;
  // return the dereferenced type of current type
  virtual TypePtr GetDerefedType() const = 0;
  // return the deconsted type of current type
  virtual TypePtr GetDeconstedType() const = 0;
  // return the identifier of current type
  virtual std::string GetTypeId() const = 0;
  // return a new type with specific value type (left/right)
  virtual TypePtr GetValueType(bool is_right) const = 0;
  // return a new trivial type
  // i.e. all left values, no constants,
  //      replace enumerations with integers,
  virtual TypePtr GetTrivialType() const = 0;

  // setters
  static void set_ptr_size(std::size_t ptr_size) { ptr_size_ = ptr_size; }

  // getters
  static std::size_t ptr_size() { return ptr_size_; }

 private:
  // size of pointer
  static std::size_t ptr_size_;
};

class PrimType : public BaseType {
 public:
  enum class Type {
    Void,
    Int8, Int32,
    UInt8, UInt32,
  };

  PrimType(Type type, bool is_right) : type_(type), is_right_(is_right) {}

  bool IsRightValue() const override { return is_right_; }
  bool IsVoid() const override { return type_ == Type::Void; }
  bool IsInteger() const override {
    auto t = static_cast<int>(type_);
    return t >= static_cast<int>(Type::Int8) &&
           t <= static_cast<int>(Type::UInt32);
  }
  bool IsUnsigned() const override {
    auto t = static_cast<int>(type_);
    return t >= static_cast<int>(Type::UInt8) &&
           t <= static_cast<int>(Type::UInt32);
  }
  bool IsStruct() const override { return false; }
  bool IsConst() const override { return false; }
  bool IsFunction() const override { return false; }
  bool IsArray() const override { return false; }
  bool IsPointer() const override { return false; }
  std::size_t GetAlignSize() const override { return GetSize(); }
  std::optional<TypePtrList> GetArgsType() const override { return {}; }
  TypePtr GetReturnType(const TypePtrList &args) const override {
    return nullptr;
  }
  std::size_t GetLength() const override { return 0; }
  TypePtr GetElem(std::size_t index) const override { return nullptr; }
  TypePtr GetElem(const std::string &name) const override {
    return nullptr;
  }
  std::optional<std::size_t> GetElemIndex(
      const std::string &name) const override {
    return {};
  }
  TypePtr GetDerefedType() const override { return nullptr; }
  TypePtr GetDeconstedType() const override { return nullptr; }
  TypePtr GetTrivialType() const override { return GetValueType(false); }

  bool CanAccept(const TypePtr &type) const override;
  bool CanCastTo(const TypePtr &type) const override;
  bool IsIdentical(const TypePtr &type) const override;
  std::size_t GetSize() const override;
  std::string GetTypeId() const override;
  TypePtr GetValueType(bool is_right) const override;

 private:
  Type type_;
  bool is_right_;
};

class StructType : public BaseType {
 public:
  StructType(TypePairList elems, const std::string &id, bool is_right)
      : elems_(std::move(elems)), id_(id), is_right_(is_right) {
    CalcSize();
  }

  bool IsRightValue() const override { return is_right_; }
  bool IsVoid() const override { return false; }
  bool IsInteger() const override { return false; }
  bool IsUnsigned() const override { return false; }
  bool IsStruct() const override { return true; }
  bool IsConst() const override { return false; }
  bool IsFunction() const override { return false; }
  bool IsArray() const override { return false; }
  bool IsPointer() const override { return false; }
  bool CanCastTo(const TypePtr &type) const override {
    return IsIdentical(type);
  }
  std::size_t GetSize() const override { return size_; }
  std::size_t GetAlignSize() const override { return base_size_; }
  std::optional<TypePtrList> GetArgsType() const override { return {}; }
  TypePtr GetReturnType(const TypePtrList &args) const override {
    return nullptr;
  }
  std::size_t GetLength() const override { return elems_.size(); }
  TypePtr GetElem(std::size_t index) const override {
    return elems_[index].second;
  }
  TypePtr GetDerefedType() const override { return nullptr; }
  TypePtr GetDeconstedType() const override { return nullptr; }
  std::string GetTypeId() const override { return id_; }

  bool CanAccept(const TypePtr &type) const override;
  bool IsIdentical(const TypePtr &type) const override;
  TypePtr GetElem(const std::string &name) const override;
  std::optional<std::size_t> GetElemIndex(
      const std::string &name) const override;
  TypePtr GetValueType(bool is_right) const override;
  TypePtr GetTrivialType() const override;

  // setters
  void set_elems(TypePairList elems) {
    elems_ = std::move(elems);
    CalcSize();
  }

 private:
  void CalcSize();

  TypePairList elems_;
  std::string id_;
  bool is_right_;
  std::size_t size_, base_size_;
};

class ConstType : public BaseType {
 public:
  ConstType(TypePtr type) : type_(type) {}

  bool IsRightValue() const override { return type_->IsRightValue(); }
  bool IsVoid() const override { return type_->IsVoid(); }
  bool IsInteger() const override { return type_->IsInteger(); }
  bool IsUnsigned() const override { return type_->IsUnsigned(); }
  bool IsStruct() const override { return type_->IsStruct(); }
  bool IsConst() const override { return true; }
  bool IsFunction() const override { return type_->IsFunction(); }
  bool IsArray() const override { return type_->IsArray(); }
  bool IsPointer() const override { return type_->IsPointer(); }
  bool CanAccept(const TypePtr &type) const override { return false; }
  bool CanCastTo(const TypePtr &type) const override {
    return type_->CanCastTo(type->IsConst() ? type->GetDeconstedType()
                                            : type);
  }
  bool IsIdentical(const TypePtr &type) const override {
    return type_->IsIdentical(type->IsConst() ? type->GetDeconstedType()
                                              : type);
  }
  std::size_t GetSize() const override { return type_->GetSize(); }
  std::size_t GetAlignSize() const override {
    return type_->GetAlignSize();
  }
  std::optional<TypePtrList> GetArgsType() const override {
    return type_->GetArgsType();
  }
  TypePtr GetReturnType(const TypePtrList &args) const override {
    return type_->GetReturnType(args);
  }
  std::size_t GetLength() const override { return type_->GetLength(); }
  std::optional<std::size_t> GetElemIndex(
      const std::string &name) const override {
    return type_->GetElemIndex(name);
  }
  TypePtr GetDerefedType() const override {
    return type_->GetDerefedType();
  }
  TypePtr GetDeconstedType() const override { return type_; }
  std::string GetTypeId() const override {
    return type_->GetTypeId();
  }
  TypePtr GetTrivialType() const override {
    return type_->GetTrivialType();
  }

  TypePtr GetElem(std::size_t index) const override;
  TypePtr GetElem(const std::string &name) const override;
  TypePtr GetValueType(bool is_right) const override;

 private:
  TypePtr type_;
};

class FuncType : public BaseType {
 public:
  FuncType(TypePtrList args, TypePtr ret, bool is_right)
      : args_(std::move(args)), ret_(std::move(ret)),
        is_right_(is_right) {}

  bool IsRightValue() const override { return is_right_; }
  bool IsVoid() const override { return false; }
  bool IsInteger() const override { return false; }
  bool IsUnsigned() const override { return false; }
  bool IsStruct() const override { return false; }
  bool IsConst() const override { return false; }
  bool IsFunction() const override { return true; }
  bool IsArray() const override { return false; }
  bool IsPointer() const override { return false; }
  std::size_t GetAlignSize() const override { return GetSize(); }
  std::optional<TypePtrList> GetArgsType() const override { return args_; }
  std::size_t GetLength() const override { return 0; }
  TypePtr GetElem(std::size_t index) const override { return nullptr; }
  TypePtr GetElem(const std::string &name) const override {
    return nullptr;
  }
  std::optional<std::size_t> GetElemIndex(
      const std::string &name) const override {
    return {};
  }
  TypePtr GetDerefedType() const override { return nullptr; }
  TypePtr GetDeconstedType() const override { return nullptr; }

  bool CanAccept(const TypePtr &type) const override;
  bool CanCastTo(const TypePtr &type) const override;
  bool IsIdentical(const TypePtr &type) const override;
  std::size_t GetSize() const override;
  TypePtr GetReturnType(const TypePtrList &args) const override;
  std::string GetTypeId() const override;
  TypePtr GetValueType(bool is_right) const override;
  TypePtr GetTrivialType() const override;

 private:
  TypePtrList args_;
  TypePtr ret_;
  bool is_right_;
};

class ArrayType : public BaseType {
 public:
  ArrayType(TypePtr base, std::size_t len, bool is_right)
      : base_(std::move(base)), len_(len), is_right_(is_right) {}

  bool IsRightValue() const override { return is_right_; }
  bool IsVoid() const override { return false; }
  bool IsInteger() const override { return false; }
  bool IsUnsigned() const override { return false; }
  bool IsStruct() const override { return false; }
  bool IsConst() const override { return false; }
  bool IsFunction() const override { return false; }
  bool IsArray() const override { return true; }
  bool IsPointer() const override { return false; }
  std::size_t GetSize() const override {
    return base_->GetSize() * len_;
  }
  std::size_t GetAlignSize() const override {
    return base_->GetAlignSize();
  }
  std::optional<TypePtrList> GetArgsType() const override { return {}; }
  TypePtr GetReturnType(const TypePtrList &args) const override {
    return nullptr;
  }
  std::size_t GetLength() const override { return len_; }
  TypePtr GetElem(std::size_t index) const override { return base_; }
  TypePtr GetElem(const std::string &name) const override {
    return nullptr;
  }
  std::optional<std::size_t> GetElemIndex(
      const std::string &name) const override {
    return {};
  }
  TypePtr GetDerefedType() const override { return base_; }
  TypePtr GetDeconstedType() const override { return nullptr; }

  bool CanAccept(const TypePtr &type) const override;
  bool CanCastTo(const TypePtr &type) const override;
  bool IsIdentical(const TypePtr &type) const override;
  std::string GetTypeId() const override;
  TypePtr GetValueType(bool is_right) const override;
  TypePtr GetTrivialType() const override;

 private:
  TypePtr base_;
  std::size_t len_;
  bool is_right_;
};

class PointerType : public BaseType {
 public:
  PointerType(TypePtr base, bool is_right)
      : base_(std::move(base)), is_right_(is_right) {}

  bool IsRightValue() const override { return is_right_; }
  bool IsVoid() const override { return false; }
  bool IsInteger() const override { return false; }
  bool IsUnsigned() const override { return false; }
  bool IsStruct() const override { return false; }
  bool IsConst() const override { return false; }
  bool IsFunction() const override { return false; }
  bool IsArray() const override { return false; }
  bool IsPointer() const override { return true; }
  std::size_t GetAlignSize() const override { return GetSize(); }
  std::optional<TypePtrList> GetArgsType() const override { return {}; }
  TypePtr GetReturnType(const TypePtrList &args) const override {
    return nullptr;
  }
  std::size_t GetLength() const override { return 0; }
  TypePtr GetElem(std::size_t index) const override { return nullptr; }
  TypePtr GetElem(const std::string &name) const override {
    return nullptr;
  }
  std::optional<std::size_t> GetElemIndex(
      const std::string &name) const override {
    return {};
  }
  TypePtr GetDerefedType() const override { return base_; }
  TypePtr GetDeconstedType() const override { return nullptr; }

  bool CanAccept(const TypePtr &type) const override;
  bool CanCastTo(const TypePtr &type) const override;
  bool IsIdentical(const TypePtr &type) const override;
  std::size_t GetSize() const override;
  std::string GetTypeId() const override;
  TypePtr GetValueType(bool is_right) const override;
  TypePtr GetTrivialType() const override;

 private:
  TypePtr base_;
  bool is_right_;
};

// create a new primitive type
inline TypePtr MakePrimType(PrimType::Type type, bool is_right) {
  return std::make_shared<PrimType>(type, is_right);
}

// create a new void type
inline TypePtr MakeVoid() {
  return std::make_shared<PrimType>(PrimType::Type::Void, true);
}

// create a new pointer type
inline TypePtr MakePointer(const TypePtr &type, bool is_right) {
  return std::make_shared<PointerType>(type, is_right);
}

// create a new pointer type (right value)
inline TypePtr MakePointer(const TypePtr &type) {
  return std::make_shared<PointerType>(type, true);
}

// get common type of two specific types
// perform implicit casting of integer types
inline const TypePtr &GetCommonType(const TypePtr &t1, const TypePtr &t2) {
  assert(t1->IsInteger() && t2->IsInteger());
  if (t1->GetSize() != t2->GetSize()) {
    return t1->GetSize() > t2->GetSize() ? t1 : t2;
  }
  else {
    return t1->IsUnsigned() ? t1 : t2;
  }
}

}  // namespace mimic::define

#endif  // MIMIC_DEFINE_TYPE_H_
