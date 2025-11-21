#ifndef BADTL_PRINT_HPP
#define BADTL_PRINT_HPP

#include <cstdio>

#include "types.hpp"

namespace btl {

inline void format(usize& buf_idx, char* buffer, usize n, const char* fmt);
template <typename T, typename... Ts>
void format(usize& buf_idx, char* buffer, usize n, const char* fmt, const T& first, const Ts&... args);
template <typename... Ts>
void print(const char* fmt, const Ts&... args);

}

namespace btl {

struct String;

template <typename T>
inline static void write_formatted_type(usize& buf_idx, char* buffer, usize n, const T& first) {
  auto x = to_string(first);
  for (const auto c : x) {
    if (buf_idx + 1 >= n) {
      break;
    }
    buffer[buf_idx++] = c;
  }
  (void) n;
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, const btl::i32& first) {
  auto written = std::snprintf(buffer + buf_idx, n - buf_idx - 1, "%d", first);
  buf_idx += static_cast<usize>(written);
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, const btl::u32& first) {
  auto written = std::snprintf(buffer + buf_idx, n - buf_idx - 1, "%u", first);
  buf_idx += static_cast<usize>(written);
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, const btl::usize& first) {
  auto written = std::snprintf(buffer + buf_idx, n - buf_idx - 1, "%zu", first);
  buf_idx += static_cast<usize>(written);
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, const btl::f32& first) {
  auto written = std::snprintf(buffer + buf_idx, n - buf_idx - 1, "%f", static_cast<double>(first));
  buf_idx += static_cast<usize>(written);
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, const char* first) {
  auto written = std::snprintf(buffer + buf_idx, n - buf_idx - 1, "%s", first);
  buf_idx += static_cast<usize>(written);
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, bool first) {
  if (first) {
    auto written = std::snprintf(buffer + buf_idx, n - buf_idx - 1, "true");
    buf_idx += static_cast<usize>(written);
  } else {
    auto written = std::snprintf(buffer + buf_idx, n - buf_idx - 1, "false");
    buf_idx += static_cast<usize>(written);
  }
}

inline void format(usize& buf_idx, char* buffer, usize n, const char* fmt) {
  (void) n;
  while (*fmt) {
    if (buf_idx + 1 >= n) {
      return;
    }
    buffer[buf_idx++] = *fmt++;
  }
}

// TODO(szulf): probably buggy and unsafe, but dont care for now
template <typename T, typename... Ts>
void format(usize& buf_idx, char* buffer, usize n, const char* fmt, const T& first, const Ts&... args) {
  while (*fmt) {
    if (buf_idx + 1 >= n) {
      return;
    }
    if (*fmt == '{') {
      ++fmt;
      if (*fmt == '}') {
        ++fmt;
        write_formatted_type(buf_idx, buffer, n, first);
        return format(buf_idx, buffer, n, fmt, args...);
      } else {
        return;
      }
    } else {
      buffer[buf_idx++] = *fmt++;
    }
  }
}

template <typename... Ts>
void print(const char* fmt, const Ts&... args) {
  char buf[4096] = {};
  usize buf_idx = 0;
  format(buf_idx, buf, 4096, fmt, args...);
  buf[4095] = 0;
  std::fputs(buf, stdout);
}

}

#endif
