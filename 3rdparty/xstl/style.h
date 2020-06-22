#ifndef XSTL_STYLE_H_
#define XSTL_STYLE_H_

#include <ostream>
#include <utility>
#include <cassert>

// set output style of 'std::ostream'

namespace xstl {

enum class Style {
  // font style
  Bold = 1,
  Dim = 2,
  Underlined = 4,
  Blink = 5,
  Invert = 7,
  Hidden = 8,
  // foreground
  Default = 39,
  Black = 30,
  Red, Green, Yellow, Blue, Purple, Cyan, LightGray,
  DarkGray = 90,
  LightRed, LightGreen, LightYellow, LightBlue, Pink, LightCyan, White,
  // background
  DefaultBg = 49,
  BlackBg = 40,
  RedBg, GreenBg, YellowBg, BlueBg, PurpleBg, CyanBg, LightGrayBg,
  DarkGrayBg = 100,
  LightRedBg, LightGreenBg, LightYellowBg, LightBlueBg, PinkBg,
  LightCyanBg, WhiteBg,
};

// type definitions
using ResetController = void(Style);
struct StyleFormat {
  const char *format;
};

// output stream wrapper
class StreamWrapper {
 public:
  explicit StreamWrapper(std::ostream &os) : os_(os), is_reset_(false) {}
  ~StreamWrapper() { ResetStyle(); }

  template <typename T>
  StreamWrapper &operator<<(T &&arg) {
    os_ << std::forward<T>(arg);
    return *this;
  }

  // reset
  std::ostream &operator<<(ResetController &rc) {
    ResetStyle();
    return os_;
  }

  // style format
  StreamWrapper &operator<<(StyleFormat &&sf) {
    bool first_color = true;
    auto process_color = [this, &first_color](Style color) {
      if (first_color) {
        operator<<(color);
        first_color = false;
      }
      else {
        operator<<(static_cast<Style>(static_cast<int>(color) + 10));
      }
    };
    for (int i = 0; sf.format[i]; ++i) {
      switch (sf.format[i]) {
        case '_': continue;
        case 'R': ResetStyle(); break;
        case 'B': operator<<(Style::Bold); break;
        case 'D': operator<<(Style::Dim); break;
        case 'U': operator<<(Style::Underlined); break;
        case 'L': operator<<(Style::Blink); break;
        case 'I': operator<<(Style::Invert); break;
        case 'H': operator<<(Style::Hidden); break;
        case 'd': process_color(Style::Default); break;
        case 'k': process_color(Style::Black); break;
        case 'r': process_color(Style::Red); break;
        case 'g': process_color(Style::Green); break;
        case 'y': process_color(Style::Yellow); break;
        case 'b': process_color(Style::Blue); break;
        case 'm': process_color(Style::Purple); break;
        case 'c': process_color(Style::Cyan); break;
        case 'a': process_color(Style::DarkGray); break;
        case 'p': process_color(Style::Pink); break;
        case 'w': process_color(Style::White); break;
        default: assert(false); break;
      }
    }
    return *this;
  }

  // other styles
  StreamWrapper &operator<<(Style style) {
    is_reset_ = false;
    os_ << "\033[" << static_cast<int>(style) << "m";
    return *this;
  }

 private:
  void ResetStyle() {
    if (is_reset_) return;
    os_ << "\033[0m";
    is_reset_ = true;
  }

  std::ostream &os_;
  bool is_reset_;
};

// reset all of the styles
inline void reset(Style) {}

/*
 * format string (<STYLE>+<fg>[<bg>])
 *    _: dummy,   R: reset,
 *    B: bold,    D: dim,     U: underlined,
 *    L: blink,   I: invert,  H: hidden,
 *    d: default, k: black,   r: red,
 *    g: green,   y: yellow,  b: blue,
 *    m: purple,  c: cyan,    a: dark gray,
 *    p: pink,    w: white
 */
inline StyleFormat style(const char *fmt) { return {fmt}; }

}  // namespace xstl

inline xstl::StreamWrapper operator<<(std::ostream &os,
                                      xstl::StyleFormat &&sf) {
  xstl::StreamWrapper sw(os);
  sw << std::move(sf);
  return sw;
}

inline xstl::StreamWrapper operator<<(std::ostream &os, xstl::Style s) {
  xstl::StreamWrapper sw(os);
  sw << s;
  return sw;
}

#endif  // XSTL_STYLE_H_
