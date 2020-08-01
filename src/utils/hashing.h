#ifndef MIMIC_UTILS_HASHING_H_
#define MIMIC_UTILS_HASHING_H_

#include <functional>
#include <utility>

namespace mimic::utils {
namespace __impl {
inline void HashCombine(std::size_t &seed) {}

template <typename T, typename... Rest>
inline void HashCombine(std::size_t &seed, const T &v, Rest &&... rest) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  HashCombine(seed, std::forward<Rest>(rest)...);
}
}  // namespace __impl

// helper function for hash combining
template <typename... Values>
inline std::size_t HashCombine(Values &&... v) {
  std::size_t seed = 0;
  __impl::HashCombine(seed, std::forward<Values>(v)...);
  return seed;
}

// helper function for hash combining a sequence of values
template <typename Iter>
inline std::size_t HashCombineRange(Iter first, Iter last) {
  std::size_t seed = 0;
  for (auto it = first; it != last; ++it) __impl::HashCombine(seed, *it);
  return seed;
}

}  // namespace mimic::utils

namespace std {

// hasher for std::pair
template <typename T0, typename T1>
struct hash<std::pair<T0, T1>> {
  inline std::size_t operator()(const std::pair<T0, T1> &val) const {
    return mimic::utils::HashCombine(val.first, val.second);
  }
};

}  // namespace std

#endif  // MIMIC_UTILS_HASHING_H_
