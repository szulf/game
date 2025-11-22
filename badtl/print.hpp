#ifndef BADTL_PRINT_HPP
#define BADTL_PRINT_HPP

#include <stdio.h>

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
inline static void write_formatted_type(usize& buf_idx, char* buffer, usize n, const T&) {
  buf_idx += static_cast<usize>(snprintf(buffer + buf_idx, n - buf_idx, "<unsupported>"));
}

inline static void write_formatted_type(usize& buf_idx, char* buffer, usize n, void* first) {
  if (first) {
    buf_idx += static_cast<usize>(snprintf(buffer + buf_idx, n - buf_idx, "%p", first));
  } else {
    buf_idx += static_cast<usize>(snprintf(buffer + buf_idx, n - buf_idx, "<null>"));
  }
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, const btl::i32& first) {
  buf_idx += static_cast<usize>(snprintf(buffer + buf_idx, n - buf_idx, "%d", first));
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, const btl::u32& first) {
  buf_idx += static_cast<usize>(snprintf(buffer + buf_idx, n - buf_idx, "%u", first));
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, const btl::usize& first) {
  buf_idx += static_cast<usize>(snprintf(buffer + buf_idx, n - buf_idx, "%zu", first));
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, const btl::f32& first) {
  buf_idx += static_cast<usize>(snprintf(buffer + buf_idx, n - buf_idx, "%f", static_cast<double>(first)));
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, const char* first) {
  buf_idx += static_cast<usize>(snprintf(buffer + buf_idx, n - buf_idx, "%s", first));
}

inline void write_formatted_type(usize& buf_idx, char* buffer, usize n, bool first) {
  if (first) {
    buf_idx += static_cast<usize>(snprintf(buffer + buf_idx, n - buf_idx, "true"));
  } else {
    buf_idx += static_cast<usize>(snprintf(buffer + buf_idx, n - buf_idx, "false"));
  }
}

inline void format(usize& buf_idx, char* buffer, usize n, const char* fmt) {
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
  format(buf_idx, buf, sizeof(buf), fmt, args...);
  buf[sizeof(buf) - 1] = '\0';
  fputs(buf, stdout);
}

template <typename... Ts>
void println(const char* fmt, const Ts&... args) {
  char buf[4096] = {};
  usize buf_idx = 0;
  format(buf_idx, buf, sizeof(buf), fmt, args...);
  buf[sizeof(buf) - 1] = '\0';
  puts(buf);
}

}

#endif
