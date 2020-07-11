#ifndef MIMIC_FRONT_LOGGER_H_
#define MIMIC_FRONT_LOGGER_H_

#include <string_view>
#include <memory>
#include <cstddef>

namespace mimic::front {

class Logger {
 public:
  Logger() : line_pos_(1), col_pos_(1) {}
  Logger(std::size_t line_pos, std::size_t col_pos)
      : line_pos_(line_pos), col_pos_(col_pos) {}

  static void ResetErrorNum(bool warn_as_err) {
    error_num_ = 0;
    warning_num_ = 0;
    warn_as_err_ = warn_as_err;
  }

  // print error message (without file info) to stderr
  static void LogRawError(std::string_view message);

  // print error message to stderr
  void LogError(std::string_view message) const;
  // print error message to stderr (with identifier)
  void LogError(std::string_view message, std::string_view id) const;
  // print warning message to stderr
  void LogWarning(std::string_view message) const;

  // reset line & column position
  void Reset() {
    line_pos_ = 1;
    col_pos_ = 1;
  }
  // increase line position
  void IncreaseLinePos() {
    ++line_pos_;
    col_pos_ = 1;
  }
  // increase column position
  void IncreaseColPos() { ++col_pos_; }

  // setters
  static void set_file(std::string_view file) { file_ = file; }

  // getters
  std::size_t line_pos() const { return line_pos_; }
  std::size_t col_pos() const { return col_pos_; }
  static std::size_t error_num() { return error_num_; }
  static std::size_t warning_num() { return warning_num_; }

 private:
  void LogFileInfo() const;

  static std::string_view file_;
  static std::size_t error_num_, warning_num_;
  static bool warn_as_err_;
  std::size_t line_pos_, col_pos_;
};

// pointer of logger
using LogPtr = std::shared_ptr<Logger>;

}  // namespace mimic::front

#endif  // MIMIC_FRONT_LOGGER_H_
