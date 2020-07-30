#ifndef XSTL_CAST_H_
#define XSTL_CAST_H_

#include <cstddef>
#include <cstdint>

namespace xstl {

// get int type by specifying bit width
template <std::size_t N> struct IntPtrType { using Type = void; };
template <> struct IntPtrType<8> { using Type = std::uint8_t; };
template <> struct IntPtrType<16> { using Type = std::uint16_t; };
template <> struct IntPtrType<32> { using Type = std::uint32_t; };
template <> struct IntPtrType<64> { using Type = std::uint64_t; };

template <std::size_t N, typename T>
typename IntPtrType<N>::Type *IntPtrCast(T *ptr) {
  return reinterpret_cast<typename IntPtrType<N>::Type *>(ptr);
}

template <std::size_t N, typename T>
const typename IntPtrType<N>::Type *IntPtrCast(const T *ptr) {
  return reinterpret_cast<const typename IntPtrType<N>::Type *>(ptr);
}

template <typename T, typename U>
T *PtrCast(U *ptr) {
  return reinterpret_cast<T *>(ptr);
}

template <typename T, typename U>
const T *PtrCast(const U *ptr) {
  return reinterpret_cast<const T *>(ptr);
}

// conversion between different endians
inline std::uint16_t EndianCast(std::uint16_t data) {
  // 0xaabb -> 0xbbaa
  return ((data & 0xff) << 8) | (data >> 8);
}

inline std::uint32_t EndianCast(std::uint32_t data) {
  // 0xaabbccdd -> 0xddccbbaa
  return ((data & 0xff) << 24) | ((data & 0xff00) << 8) |
         ((data & 0xff0000) >> 8) | (data >> 24);
}

inline std::uint64_t EndianCast(std::uint64_t data) {
  // 0xaabbccdd11223344 -> 0x44332211ddccbbaa
  return ((data & 0xff) << 56) | ((data & 0xff00) << 40) |
         ((data & 0xff0000) << 24) | ((data & 0xff000000) << 8) |
         ((data & 0xff00000000) >> 8) | ((data & 0xff0000000000) >> 24) |
         ((data & 0xff000000000000) >> 40) | (data >> 56);
}

}  // namespace xstl

#endif  // XSTL_CAST_H_
