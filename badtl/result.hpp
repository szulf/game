#ifndef BADTL_RESULT_HPP
#define BADTL_RESULT_HPP

#include "utils.hpp"

namespace btl {

template <typename T, typename E>
struct Result {
  union Value {
    T success;
    E error;
  };

  T expect(const char* crash_msg);
  T value_or(const T& or_value);

  bool has_err;
  Value value;
};

template <typename E>
struct Result<void, E> {
  union Value {
    E error;
  };

  bool has_err;
  Value value;
};

template <typename E, typename T>
btl_always_inline Result<T, E> ok(const T& val);
template <typename E>
btl_always_inline Result<void, E> ok();
template <typename T, typename E>
btl_always_inline Result<T, E> err(E err);

}

namespace btl {

template <typename E, typename T>
btl_always_inline Result<T, E> ok(const T& val) {
  Result<T, E> out = {};
  out.has_err = false;
  out.value.success = val;
  return out;
}

template <typename E>
btl_always_inline Result<void, E> ok() {
  Result<void, E> out = {};
  out.has_err = false;
  return out;
}

template <typename T, typename E>
btl_always_inline Result<T, E> err(E err) {
  Result<T, E> out = {};
  out.has_err = true;
  out.value.error = err;
  return out;
}

template <typename T, typename E>
T Result<T, E>::expect(const char* crash_msg) {
  ASSERT(!has_err, crash_msg);
  return value.success;
}

template <typename T, typename E>
T Result<T, E>::value_or(const T& or_value) {
  if (has_err) {
    return or_value;
  }
  return value.success;
}

}

#endif
