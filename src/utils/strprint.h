#ifndef MIMIC_UTILS_STRPRINT_H_
#define MIMIC_UTILS_STRPRINT_H_

#include <ostream>
#include <string_view>
#include <iomanip>
#include <cctype>

namespace mimic::utils {

// dump character to output stream (with C-style escaped characters)
inline void DumpChar(std::ostream &os, char c) {
  switch (c) {
    case '\a':  os << "\\a";  break;
    case '\b':  os << "\\b";  break;
    case '\f':  os << "\\f";  break;
    case '\n':  os << "\\n";  break;
    case '\r':  os << "\\r";  break;
    case '\t':  os << "\\t";  break;
    case '\v':  os << "\\v'"; break;
    case '\\':  os << "\\\\"; break;
    case '\'':  os << "\\\'";    break;
    case '"':   os << "\\\""; break;
    case '\0':  os << "\\0";  break;
    default: {
      if (std::isprint(c)) {
        os << c;
      }
      else {
        os << "\\x" << std::setw(2) << std::setfill('0') << std::hex
           << static_cast<int>(c) << std::dec;
      }
      break;
    }
  }
}

// dump string to output stream (with C-style escaped characters)
inline void DumpStr(std::ostream &os, std::string_view str) {
  for (const auto &c : str) DumpChar(os, c);
}

}  // namespace mimic::utils

#endif  // MIMIC_UTILS_STRPRINT_H_
