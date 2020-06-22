#ifndef XSTL_NESTED_H_
#define XSTL_NESTED_H_

#include <unordered_map>
#include <map>
#include <memory>
#include <type_traits>
#include <utility>
#include <optional>

namespace xstl {

// declaration of nested map
template <typename K, typename V>
class NestedMap;

// pointer to nested map
template <typename K, typename V>
using NestedMapPtr = std::shared_ptr<NestedMap<K, V>>;

namespace _nested_helper {

// check if a type is hashable
template <typename T, typename = std::void_t<>>
struct IsHashable : std::false_type {};

template <typename T>
struct IsHashable<T, std::void_t<decltype(std::declval<std::hash<T>>()(
                         std::declval<T>()))>> : std::true_type {};

// type of internal map
// use 'std::unordered_map' if type 'K' is hashable,
// otherwise use 'std::map'
template <typename K, typename V>
using Map = typename std::conditional<IsHashable<K>::value,
                                      std::unordered_map<K, V>,
                                      std::map<K, V>>::type;

// base implementation of nested map
template <typename T, typename K, typename V>
class NestedMapBase {
 public:
  NestedMapBase() : outer_(nullptr) {}
  NestedMapBase(const NestedMapPtr<K, V> &outer) : outer_(outer) {}

  // add item to current map
  void AddItem(const K &key, const V &value) {
    map_.insert({key, value});
  }

  // get item
  V GetItem(const K &key, bool recursive) const {
    return static_cast<const T *>(this)->GetItemImpl(key, recursive);
  }

  // get item recursively
  V GetItem(const K &key) const { return GetItem(key, true); }

  // remove item
  bool RemoveItem(const K &key, bool recursive) {
    auto num = map_.erase(key);
    if (num) {
      return true;
    }
    else if (outer_ && recursive) {
      return outer_->RemoveItem(key);
    }
    else {
      return false;
    }
  }

  // remove item recursively
  bool RemoveItem(const K &key) { return RemoveItem(key, true); }

  // access item
  V &AccessItem(const K &key) { return map_[key]; }

  // access operator
  V &operator[](const K &key) { return map_[key]; }

  // getters
  // outer map
  const NestedMapPtr<K, V> &outer() const { return outer_; }
  // check if current map is root map
  bool is_root() const { return outer_ == nullptr; }

 private:
  friend T;

  NestedMapPtr<K, V> outer_;
  Map<K, V> map_;
};

}  // namespace _nested_helper

template <typename K, typename V>
class NestedMap
    : public _nested_helper::NestedMapBase<NestedMap<K, V>, K, V> {
 public:
  // check if value type is suitable
  static_assert(std::is_pointer<V>::value ||
                    std::is_constructible<V, std::nullptr_t>::value,
                "value type must be a pointer or can be constructed "
                "with nullptr_t");

  // constructors
  using _nested_helper::NestedMapBase<NestedMap<K, V>, K,
                                      V>::NestedMapBase;

 private:
  friend _nested_helper::NestedMapBase<NestedMap<K, V>, K, V>;

  V GetItemImpl(const K &key, bool recursive) const {
    auto it = this->map_.find(key);
    if (it != this->map_.end()) {
      return it->second;
    }
    else if (this->outer_ && recursive) {
      return this->outer_->GetItem(key);
    }
    else {
      return nullptr;
    }
  }
};

template <typename K, typename V>
class NestedMap<K, std::optional<V>>
    : public _nested_helper::NestedMapBase<NestedMap<K, std::optional<V>>,
                                           K, std::optional<V>> {
 public:
  // actual value type
  using Vl = std::optional<V>;

  // constructors
  using _nested_helper::NestedMapBase<NestedMap<K, Vl>, K,
                                      Vl>::NestedMapBase;

 private:
  friend _nested_helper::NestedMapBase<NestedMap<K, Vl>, K, Vl>;

  Vl GetItemImpl(const K &key, bool recursive) const {
    auto it = this->map_.find(key);
    if (it != this->map_.end()) {
      return it->second;
    }
    else if (this->outer_ && recursive) {
      return this->outer_->GetItem(key);
    }
    else {
      return {};
    }
  }
};

// create a new nested map
template <typename K, typename V>
inline NestedMapPtr<K, V> MakeNestedMap() {
  return std::make_shared<NestedMap<K, V>>();
}

// create a new nested map (with outer map)
template <typename K, typename V>
inline NestedMapPtr<K, V> MakeNestedMap(const NestedMapPtr<K, V> &outer) {
  return std::make_shared<NestedMap<K, V>>(outer);
}

}  // namespace xstl

#endif  // XSTL_NESTED_H_
