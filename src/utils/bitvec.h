#ifndef MIMIC_UTILS_BITVEC_H_
#define MIMIC_UTILS_BITVEC_H_

#include <vector>
#include <ostream>
#include <cstdint>
#include <cassert>

namespace mimic::utils {

// vector that can holds N bits
class BitVec {
 public:
  // create an empty bit vector
  BitVec() : width_(0) {}
  // create a zero initialized bit vector with 'width' bits
  BitVec(std::size_t width) { Resize(width); }

  // get the specific bit
  bool Get(std::size_t i) const {
    assert(i < width_);
    return vec_[i / 64] & (static_cast<std::uint64_t>(1) << (i % 64));
  }

  // set the specific bit
  void Set(std::size_t i) {
    assert(i < width_);
    vec_[i / 64] |= static_cast<std::uint64_t>(1) << (i % 64);
  }

  // clear the specific bit
  void Clear(std::size_t i) {
    assert(i < width_);
    vec_[i / 64] &= ~(static_cast<std::uint64_t>(1) << (i % 64));
  }

  // assign to the specific bit
  void Assign(std::size_t i, bool bit) { return bit ? Set(i) : Clear(i); }

  // check if current bit vector is empty
  bool Empty() const { return !width_; }

  // set the bit width to a specific value
  void Resize(std::size_t width) {
    vec_.resize((width + 63) / 64);
    width_ = width;
  }

  // add a bit to the back of current bit vector
  void Push(bool bit) {
    if ((++width_ + 63) / 64 > vec_.size()) vec_.push_back(0);
    Assign(width_ - 1, bit);
  }

  // remove the last bit of current bit vector
  void Pop() {
    assert(width_);
    if ((--width_ + 63) / 64 < vec_.size()) vec_.pop_back();
  }

  // remove all stored bits, set width to zero
  void PopAll() {
    vec_.clear();
    width_ = 0;
  }

  // clear all stored bits
  void Clear() { Fill(false); }

  // fill the bit vector with the specific bit
  void Fill(bool bit) {
    for (auto &&i : vec_) i = bit ? static_cast<std::uint64_t>(-1) : 0;
  }
  
  // flip all stored bits
  void Flip() {
    for (auto &&i : vec_) i = ~i;
  }

  // merge with another bit vector
  // the width of two bit vectors must be the same
  void Merge(const BitVec &bv) {
    assert(width_ == bv.width_);
    for (std::size_t i = 0; i < vec_.size(); ++i) {
      vec_[i] |= bv.vec_[i];
    }
  }

  // calculate intersection of two bit vectors
  // the width of two bit vectors must be the same
  void Intersect(const BitVec &bv) {
    assert(width_ == bv.width_);
    for (std::size_t i = 0; i < vec_.size(); ++i) {
      vec_[i] &= bv.vec_[i];
    }
  }

  // check if two bit vectors are equal
  bool operator==(const BitVec &rhs) const {
    if (width_ != rhs.width_) return false;
    if (!width_) return true;
    for (std::size_t i = 0; i < vec_.size() - 1; ++i) {
      if (vec_[i] != rhs.vec_[i]) return false;
    }
    auto shift = 64 - (width_ - 1) % 64 - 1;
    auto mask = static_cast<std::uint64_t>(-1) >> shift;
    return (vec_.back() & mask) == (rhs.vec_.back() & mask);
  }
  bool operator!=(const BitVec &rhs) const { return !(*this == rhs); }

  // flip operation
  BitVec operator~() const {
    auto bv = *this;
    bv.Flip();
    return bv;
  }

  // merge operation
  BitVec &operator|=(const BitVec &rhs) {
    Merge(rhs);
    return *this;
  }
  BitVec operator|(const BitVec &rhs) const {
    auto bv = *this;
    bv.Merge(rhs);
    return bv;
  }

  // intersetion
  BitVec &operator&=(const BitVec &rhs) {
    Intersect(rhs);
    return *this;
  }
  BitVec operator&(const BitVec &rhs) const {
    auto bv = *this;
    bv.Intersect(rhs);
    return bv;
  }

  // stream
  friend std::ostream &operator<<(std::ostream &os, const BitVec &bv) {
    os << '(';
    for (std::size_t i = 0; i < bv.width_; ++i) {
      os << static_cast<int>(bv.Get(i));
    }
    os << ')';
    return os;
  }

  // getters
  std::size_t width() const { return width_; }

 private:
  std::vector<std::uint64_t> vec_;
  std::size_t width_;
};

}  // namespace mimic::utils

#endif  // MIMIC_UTILS_BITVEC_H_
