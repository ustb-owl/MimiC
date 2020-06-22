#ifndef XSTL_BITPAT_H_
#define XSTL_BITPAT_H_

#include <type_traits>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <variant>
#include <optional>
#include <utility>
#include <initializer_list>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cassert>

namespace xstl {

// bit pattern
template <typename T>
class BitPat {
 public:
  static_assert(std::is_integral<T>::value);

  constexpr BitPat(T value, T mask) : value_(value), mask_(mask) {}
  constexpr BitPat(T value) : value_(value), mask_(-1) {}
  constexpr BitPat(std::string_view s)
      : value_(GetValue(s)), mask_(GetMask(s)) {}

  constexpr bool operator==(const BitPat<T> &rhs) const {
    auto mask = mask_ & rhs.mask_;
    return (value_ & mask) == (rhs.value_ & mask);
  }
  constexpr bool operator!=(const BitPat<T> &rhs) const {
    return !operator==(rhs);
  }

  // getters
  constexpr T value() const { return value_; }
  constexpr T mask() const { return mask_; }

 private:
  // get value from string
  constexpr T GetValue(std::string_view s) const {
    assert(s.size() == sizeof(T) * 8);
    T cur_bit = static_cast<T>(1) << (s.size() - 1), v = 0;
    for (const auto &c : s) {
      if (c == '1') v |= cur_bit;
      cur_bit >>= 1;
    }
    return v;
  }

  // get mask from string
  constexpr T GetMask(std::string_view s) const {
    assert(s.size() == sizeof(T) * 8);
    T cur_bit = static_cast<T>(1) << (s.size() - 1), m = 0;
    for (const auto &c : s) {
      if (c == '0' || c == '1') m |= cur_bit;
      cur_bit >>= 1;
    }
    return m;
  }

  T value_, mask_;
};

// bit pattern for 32-bit data
using BitPat32 = BitPat<std::uint32_t>;


// tool for bitwise pattern matching
template <typename T, typename Val>
class BitMatch {
 public:
  static_assert(std::is_integral<T>::value);

  using InitPair = std::pair<BitPat<T>, Val>;

  BitMatch() {}
  BitMatch(std::initializer_list<InitPair> init) {
    InitRules(init.begin(), init.end());
  }
  BitMatch(std::initializer_list<std::pair<std::string_view, Val>> init) {
    std::vector<InitPair> init_bp;
    for (const auto &i : init) init_bp.push_back({{i.first}, i.second});
    InitRules(init_bp);
  }

  // initialize rules by specific iterator
  template <typename It>
  bool InitRules(It begin, It end) {
    It skip = end;
    // get current mask
    mask_ = -1;
    for (It it = begin; it != end; ++it) {
      if (!it->first.mask()) {
        // add to default rule
        if (!default_val_) {
          default_val_ = {it->second};
          // mark as skipped rule
          skip = it;
        }
        else {
          return false;
        }
      }
      else {
        // update current mask
        mask_ &= it->first.mask();
      }
    }
    if (!mask_) return false;
    // get classified rules
    std::unordered_map<T, std::vector<InitPair>> rules;
    for (It it = begin; it != end; ++it) {
      // skip default rule
      if (it == skip) continue;
      // handle the rest
      const auto &p = it->first;
      auto &r = rules[p.value() & mask_];
      r.push_back({{p.value() & ~mask_, p.mask() & ~mask_}, it->second});
    }
    // divide and conquer
    for (const auto &it : rules) {
      const auto &pairs = it.second;
      if (pairs.size() == 1 && !pairs.back().first.mask()) {
        // just add as value
        matches_.insert({it.first, pairs.back().second});
      }
      else {
        // add sub-match
        auto sub = std::make_unique<BitMatch<T, Val>>();
        if (!sub->InitRules(pairs)) return false;
        matches_.insert({it.first, std::move(sub)});
      }
    }
    return true;
  }

  // initialize rules by initializer vector
  bool InitRules(const std::vector<InitPair> &init) {
    return InitRules(init.begin(), init.end());
  }

  // perform bit pattern match
  std::optional<Val> Find(T value) {
    auto it = matches_.find(value & mask_);
    if (it == matches_.end()) {
      return default_val_;
    }
    else if (auto val_ptr = std::get_if<Val>(&it->second)) {
      return {*val_ptr};
    }
    else {
      auto sub_ptr = std::get_if<SubMatch>(&it->second);
      assert(sub_ptr);
      auto ret = (*sub_ptr)->Find(value);
      return ret ? ret : default_val_;
    }
  }

  // get count of matching rules
  std::size_t GetSize() const {
    std::size_t size = default_val_ ? 1 : 0;
    for (const auto &it : matches_) {
      if (auto sub_ptr = std::get_if<SubMatch>(&it.second)) {
        size += (*sub_ptr)->GetSize();
      }
      else {
        ++size;
      }
    }
    return size;
  }

 private:
  using SubMatch = std::unique_ptr<BitMatch<T, Val>>;

  std::unordered_map<T, std::variant<Val, SubMatch>> matches_;
  std::optional<Val> default_val_;
  T mask_;
};

// bitwise pattern maching for 32-bit data
template <typename Val>
using BitMatch32 = BitMatch<std::uint32_t, Val>;

}  // namespace xstl

#endif  // XSTL_BITPAT_H_
