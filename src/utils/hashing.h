#ifndef MIMIC_UTILS_HASHING_H_
#define MIMIC_UTILS_HASHING_H_

#include <functional>
#include <utility>

namespace mimic::utils {

inline void HashCombine(std::size_t &seed) {}

// helper function for hash combining
template <typename T, typename... Rest>
inline void HashCombine(std::size_t &seed, const T &v, Rest &&... rest) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  HashCombine(seed, std::forward<Rest>(rest)...);
}

}  // namespace mimic::utils

namespace std {

// hasher for std::pair
template <typename T0, typename T1>
struct hash<std::pair<T0, T1>> {
  inline std::size_t operator()(const std::pair<T0, T1> &val) const {
    std::size_t seed = 0;
    mimic::utils::HashCombine(seed, val.first, val.second);
    return seed;
  }
};

}  // namespace std

#endif  // MIMIC_UTILS_HASHING_H_
