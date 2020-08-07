#include "opt/helper/ircopier.h"

using namespace mimic::mid;
using namespace mimic::opt;

SSAPtr IRCopier::GetCopy(const SSAPtr &value) {
  if (!value) return nullptr;
  // find from cache
  auto it = copied_vals_.find(value.get());
  if (it != copied_vals_.end()) {
    return it->second;
  }
  else {
    // tell value that we need a copy
    value->RunPass(*this);
    auto it = copied_vals_.find(value.get());
    if (it != copied_vals_.end()) {
      return it->second;
    }
    else {
      // can not copy current value (maybe a global var/func/const/...)
      // return original value and add it to cache
      return copied_vals_[value.get()] = value;
    }
  }
}

void IRCopier::CopyOperand(const UserPtr &copied, User &user) {
  for (std::size_t i = 0; i < user.size(); ++i) {
    (*copied)[i].set_value(GetCopy(user[i].value()));
  }
}
