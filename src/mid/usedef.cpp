#include "mid/usedef.h"

#include <algorithm>

using namespace mimic::mid;

void IdManager::ResetId() {
  cur_id_ = 0;
  ids_.clear();
}

std::size_t IdManager::GetId(const Value *val) {
  auto it = ids_.find(val);
  if (it == ids_.end()) {
    auto id = cur_id_++;
    ids_.insert({val, id});
    return id;
  }
  else {
    return it->second;
  }
}

void IdManager::LogName(const Value *val, std::string_view name) {
  if (names_.find(val) == names_.end()) names_.insert({val, name});
}

std::optional<std::string_view> IdManager::GetName(const Value *v) const {
  auto it = names_.find(v);
  if (it != names_.end()) {
    return it->second;
  }
  else {
    return {};
  }
}

void Value::ReplaceBy(const SSAPtr &value) {
  if (value.get() == this) return;
  // reroute all uses to new value
  while (!uses_.empty()) {
    uses_.front()->set_value(value);
  }
}

void Value::RemoveFromUser() {
  // remove from all users
  while (!uses_.empty()) {
    uses_.front()->user()->RemoveValue(this);
  }
}

void User::RemoveValue(const SSAPtr &value) {
  uses_.erase(std::remove_if(uses_.begin(), uses_.end(),
                             [&value](const Use &use) {
                               return use.value() == value;
                             }),
              uses_.end());
}

void User::RemoveValue(Value *value) {
  uses_.erase(std::remove_if(uses_.begin(), uses_.end(),
                             [&value](const Use &use) {
                               return use.value().get() == value;
                             }),
              uses_.end());
}
