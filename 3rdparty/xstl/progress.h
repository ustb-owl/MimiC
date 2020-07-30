#ifndef XSTL_PROGRESS_H_
#define XSTL_PROGRESS_H_

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <thread>
#include <cmath>

namespace xstl {

class Progress {
 public:
  Progress() : length_(70), fill_('#'), empty_(' ') {
    std::cout.sync_with_stdio(false);
    set_count(1);
  }
  Progress(int length) : length_(length), fill_('#'), empty_(' ') {
    std::cout.sync_with_stdio(false);
    set_count(1);
  }
  Progress(int length, int count)
      : length_(length), fill_('#'), empty_(' ') {
    std::cout.sync_with_stdio(false);
    set_count(count);
  }

  void Show() const {
    for (const auto &i : info_) {
      std::cout << i.title;
      std::cout << std::setw(length_ - i.title.length()) << ' ';
      std::cout << std::endl;
      int prog_cur = (length_ - 10) * i.percent;
      int prog_left = length_ - 10 - prog_cur;
      std::cout << '[' << std::setfill(fill_);
      if (prog_cur) std::cout << std::setw(prog_cur) << fill_;
      std::cout << std::setfill(empty_);
      if (prog_left) std::cout << std::setw(prog_left) << empty_;
      std::cout << std::setfill(' ');
      std::cout << "] " << std::setw(6) << std::setprecision(2);
      std::cout << std::fixed << i.percent * 100 << '%' << std::endl;
    }
  }

  void Refresh() const {
    for (int i = 0; i < count(); ++i) {
      std::cout << "\033[F\033[F";
    }
    Show();
  }

  std::thread RefreshAsync() const {
    return std::thread(&Progress::RefreshUntilFinish, this);
  }

  void set_title(const std::string &title) { set_title(0, title); }
  void set_title(int index, const std::string &title) {
    info_[index].title = title;
  }

  void set_percent(float percent) { set_percent(0, percent); }
  void set_percent(int index, float percent) {
    if (percent < 0) {
      info_[index].percent = 0;
    }
    else if (percent > 1) {
      info_[index].percent = 1;
    }
    else {
      info_[index].percent = percent;
    }
  }

  void set_length(int length) { length_ = length; }
  void set_count(int count) { info_.resize(count); }
  void set_fill(char fill) { fill_ = fill; }
  void set_empty(char empty) { empty_ = empty; }

  const std::string &title() const { return info_[0].title; }
  const std::string &title(int index) const { return info_[index].title; }
  float percent() const { return info_[0].percent; }
  float percent(int index) const { return info_[index].percent; }
  int length() const { return length_; }
  int count() const { return info_.size(); }
  char fill() const { return fill_; }
  char empty() const { return empty_; }

 private:
  struct ProgressInfo {
    ProgressInfo() : percent(0) {}
    std::string title;
    float percent;
  };

  void RefreshUntilFinish() const {
    bool finish = false;
    while (!finish) {
      finish = true;
      for (const auto &i : info_) {
        finish = finish && std::fabs(i.percent - 1) < 1e-6;
      }
      Refresh();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  int length_;
  char fill_, empty_;
  std::vector<ProgressInfo> info_;
};

}  // namespace xstl

#endif  // XSTL_PROGRESS_H_
