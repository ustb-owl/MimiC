#ifndef XSTL_ARGPARSE_H_
#define XSTL_ARGPARSE_H_

#include <string>
#include <vector>
#include <map>
#include <any>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstddef>
#include <cctype>
#include <cassert>

// all supported types
#define ARGPARSE_SUPPORTED_TYPES(f)                               \
  f(short) f(unsigned short) f(int) f(unsigned int) f(long)       \
  f(unsigned long) f(long long) f(unsigned long long) f(bool)     \
  f(float) f(double) f(long double) f(std::string)
// expand to type check
#define ARGPARSE_EXPAND_EQL(t)                                    \
  type == typeid(t) || type == typeid(std::vector<t>) ||
// expand to value read
#define ARGPARSE_EXPAND_READ(t)                                   \
  if (value.type() == typeid(t)) return ReadValue<t>(arg, value); \
  if (value.type() == typeid(std::vector<t>)) {                   \
    return ReadToVec<t>(arg, value);                              \
  }

namespace xstl {

// command line argument parser
class ArgParser {
 public:
  ArgParser() : program_name_("app"), padding_(kDefaultPadding) {}

  // add a new argument
  template <typename T>
  void AddArgument(const std::string &name, const std::string &help) {
    // check name
    CheckArgName(name);
    // check type
    CheckArgType(typeid(T));
    // update padding width
    auto txt_width = name.size() + kArgMinWidth;
    if (txt_width > padding_) padding_ = txt_width;
    // push into argument list
    args_.push_back({name, "", help});
    vals_.insert({name, T()});
  }

  // add a new option
  template <typename T>
  void AddOption(const std::string &name, const std::string &short_name,
                 const std::string &help, const T &default_val) {
    // check name
    CheckArgName(name);
    CheckArgName(short_name);
    // check type
    CheckArgType(typeid(T));
    // update padding width
    auto txt_width = name.size() + short_name.size() + kArgMinWidth;
    if (txt_width > padding_) padding_ = txt_width;
    // push into option list
    opts_.push_back({name, short_name, help});
    opt_map_.insert({"--" + name, opts_.size() - 1});
    opt_map_.insert({"-" + short_name, opts_.size() - 1});
    vals_.insert({name, default_val});
  }

  // get parsed value
  template <typename T>
  T GetValue(const std::string &name) const {
    auto it = vals_.find(name);
    assert(it != vals_.end());
    auto ptr = std::any_cast<T>(&it->second);
    assert(ptr);
    return *ptr;
  }

  // parse argument
  bool Parse(int argc, const char *argv[]) {
    // update program name
    set_program_name(argv[0]);
    std::size_t arg_ofs = 0;
    // parse argument list
    for (int i = 1; i < argc; ++i) {
      if (argv[i][0] == '-') {
        // find option info
        auto it = opt_map_.find(argv[i]);
        if (it == opt_map_.end()) return false;
        const auto &info = opts_[it->second];
        auto &val = vals_[info.name];
        // fill value
        if (val.type() == typeid(bool)) {
          val = true;
        }
        else {
          // read argument of option
          ++i;
          if (i >= argc) return false;
          if (argv[i][0] == '-' || !ReadArgValue(argv[i], val)) {
            return false;
          }
        }
      }
      else {
        // parse an argument
        if (arg_ofs >= args_.size()) return false;
        if (!ReadArgValue(argv[i], vals_[args_[arg_ofs].name])) {
          return false;
        }
        ++arg_ofs;
      }
    }
    return arg_ofs == args_.size();
  }

  // print help message
  void PrintHelp() {
    // print usage
    std::cout << "Usage: " << program_name_ << " ";
    for (const auto &i : args_) {
      std::cout << "<";
      for (const auto &c : i.name) {
        std::cout << static_cast<char>(std::toupper(c));
      }
      std::cout << "> ";
    }
    if (!opts_.empty()) std::cout << "[OPTIONS...]" << std::endl;
    // print arguments
    if (!args_.empty()) {
      std::cout << std::endl;
      std::cout << "Arguments:" << std::endl;
      for (const auto &i : args_) {
        std::cout << std::left << std::setw(padding_) << ("  " + i.name);
        std::cout << i.help_msg << std::endl;
      }
    }
    // print options
    if (!opts_.empty()) {
      std::cout << std::endl;
      std::cout << "Options:" << std::endl;
      for (const auto &i : opts_) {
        // generate option title
        auto title = "  -" + i.short_name + ", --" + i.name;
        if (vals_[i.name].type() != typeid(bool)) title += " <ARG>";
        // print info
        std::cout << std::left << std::setw(padding_) << title;
        std::cout << i.help_msg << std::endl;
      }
    }
  }

  // setter
  void set_program_name(const char *program_name) {
    program_name_ = program_name;
    auto pos = program_name_.rfind('/');
    if (pos != std::string::npos) {
      program_name_ = program_name_.substr(pos + 1);
    }
  }

  // getter
  const std::string &program_name() const { return program_name_; }

 private:
  // default padding width of argument description info
  static const int kDefaultPadding = 10;
  // minimum width of argument display in help message
  static const int kArgMinWidth = sizeof("  -, -- <ARG>   ") - 1;

  struct ArgInfo {
    std::string name;
    std::string short_name;
    std::string help_msg;
  };

  template <typename T>
  bool ReadValue(const char *str, std::any &value) {
    std::istringstream iss(str);
    T v;
    iss >> v;
    value = v;
    return !iss.fail();
  }

  template <typename T>
  bool ReadToVec(const char *str, std::any &value) {
    std::istringstream iss(str);
    T v;
    iss >> v;
    std::any_cast<std::vector<T>>(&value)->push_back(v);
    return !iss.fail();
  }

  // assert argument/option name is valid
  void CheckArgName(const std::string &name) {
    // regular expression: ([A-Za-z0-9]|_|-)+
    assert(!name.empty());
    for (const auto &i : name) {
      assert(std::isalnum(i) || i == '_' || i == '-');
      static_cast<void>(i);
    }
  }

  // assert argument/option type is valid
  void CheckArgType(const std::type_info &type) {
    assert(ARGPARSE_SUPPORTED_TYPES(ARGPARSE_EXPAND_EQL) 0);
  }

  // read argument value
  bool ReadArgValue(const char *arg, std::any &value) {
    ARGPARSE_SUPPORTED_TYPES(ARGPARSE_EXPAND_READ);
    assert(false);
    return false;
  }

  std::string program_name_;
  std::size_t padding_;
  std::vector<ArgInfo> args_, opts_;
  std::map<std::string, std::size_t> opt_map_;
  std::map<std::string, std::any> vals_;
};

}  // namespace xstl

#undef ARGPARSE_SUPPORTED_TYPES
#undef ARGPARSE_EXPAND_EQL
#undef ARGPARSE_EXPAND_READ

#endif  // XSTL_ARGPARSE_H_
