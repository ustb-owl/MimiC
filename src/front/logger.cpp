#include "front/logger.h"

#include <iostream>

#include "xstl/style.h"

using namespace mimic::front;

// definition of static member variables in logger
std::string_view Logger::file_;
std::size_t Logger::error_num_, Logger::warning_num_;
bool Logger::enable_warn_, Logger::warn_as_err_;

void Logger::LogFileInfo() const {
  using namespace xstl;
  std::cerr << style("B") << file_ << ":";
  std::cerr << style("B") << line_pos_ << ":" << col_pos_ << ": ";
}

void Logger::LogRawError(std::string_view message) {
  using namespace xstl;
  // print error message
  std::cerr << style("Br") << "error: ";
  std::cerr << message << std::endl;
  // increase error number
  ++error_num_;
}

void Logger::LogError(std::string_view message) const {
  LogFileInfo();
  LogRawError(message);
}

void Logger::LogError(std::string_view message,
                      std::string_view id) const {
  using namespace xstl;
  LogFileInfo();
  // print error message
  std::cerr << style("Br") << "error: ";
  std::cerr << "id: " << id << ", " << message << std::endl;
  // increase error number
  ++error_num_;
}

// print warning message to stderr
void Logger::LogWarning(std::string_view message) const {
  using namespace xstl;
  if (!enable_warn_) return;
  // log all warnings as errors
  if (warn_as_err_) {
    LogError(message);
    return;
  }
  // print warning message
  LogFileInfo();
  std::cerr << style("Bp") << "warning: ";
  std::cerr << message << std::endl;
  // increase warning number
  ++warning_num_;
}
