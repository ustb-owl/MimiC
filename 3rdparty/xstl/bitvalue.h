#ifndef XSTL_BITVALUE_H_
#define XSTL_BITVALUE_H_

#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <cassert>

namespace xstl {

// represents a sequence of bits
// can hold up to 'sizeof(T) * 8' bits
template <typename T>
class BitValue {
 public:
  static_assert(std::is_integral<T>::value);

  constexpr BitValue(T value, std::size_t width)
      : value_(value), width_(width) {
    assert(width_ && width_ <= sizeof(T) * 8);
  }

  // get a single bit from current value
  constexpr BitValue<T> Get(std::size_t i) const { return Extract(i, i); }

  // extract a subsequence of bits from current value
  constexpr BitValue<T> Extract(std::size_t hi, std::size_t lo) const {
    assert(hi < width_ && hi >= lo);
    auto width = hi - lo + 1;
    auto mask = (1 << width) - 1;
    return BitValue<T>((value_ >> lo) & mask, width);
  }

  // concatenate with another 'BitValue'
  constexpr BitValue<T> Concat(const BitValue<T> &rhs) const {
    return BitValue<T>((value() << rhs.width()) | rhs.value(),
                       width() + rhs.width());
  }

  // 'get' operator
  constexpr BitValue<T> operator[](std::size_t i) const { return Get(i); }
  constexpr BitValue<T> operator()(std::size_t i) const { return Get(i); }

  // 'extract' operator
  constexpr BitValue<T> operator()(std::size_t hi, std::size_t lo) const {
    return Extract(hi, lo);
  }

  // 'concat' operator
  constexpr BitValue<T> operator|(const BitValue<T> &rhs) const {
    return Concat(rhs);
  }

  // conversion operator
  constexpr operator T() const { return value(); }

  // getters
  constexpr T value() const { return value_ & ((1 << width_) - 1); }
  constexpr std::size_t width() const { return width_; }

 private:
  T value_;
  std::size_t width_;
};

using BitValue32 = BitValue<std::uint32_t>;

}  // namespace xstl

#endif  // XSTL_BITVALUE_H_
