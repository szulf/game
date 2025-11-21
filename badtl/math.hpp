#ifndef BADTL_MATH_HPP
#define BADTL_MATH_HPP

#include <cmath>

#include "types.hpp"

namespace btl {

inline f32 sin(f32 value);
inline f32 cos(f32 value);
inline f32 tan(f32 value);
template <typename T>
T abs(T value);
template <typename T>
T max(T v1, T v2);
inline f32 sqrt(f32 value);
inline f32 log10(f32 value);
inline usize pow(usize base, usize exponent);
inline i32 floor(f32 value);
inline i32 ceil(f32 value);
inline bool f32_eql(f32 a, f32 b);
inline f32 radians(f32 deg);

template <typename T>
struct constants;

template <>
struct constants<f32> {
  static constexpr f32 epsilon = 1.1920928955078125e-07;
  static constexpr f32 pi = 3.141592653589793238462643383279502884197169399375105820974944592307f;
};

template <>
struct constants<u64> {
  static constexpr u64 fnv_offset = 14695981039346656037UL;
  static constexpr u64 fnv_prime = 1099511628211UL;
};

}

namespace btl {

inline f32 sin(f32 value) {
  return std::sin(value);
}

inline f32 cos(f32 value) {
  return std::cos(value);
}

inline f32 tan(f32 value) {
  return std::tan(value);
}

template <typename T>
T abs(T value) {
  return std::abs(value);
}

template <typename T>
T max(T v1, T v2) {
  if (v1 > v2) {
    return v1;
  }
  return v2;
}

inline f32 sqrt(f32 value) {
  return std::sqrt(value);
}

inline f32 log10(f32 value) {
  return std::log10(value);
}

inline usize pow(usize base, usize exponent) {
  usize val = 1;
  for (usize i = 0; i < exponent; ++i) {
    val *= base;
  }
  return val;
}

// NOTE(szulf): this probably loses some precision, but im casting to i32 in the middle so it would lose it anyway
inline i32 floor(f32 value) {
  return static_cast<i32>(value);
}

// NOTE(szulf): this probably loses some precision, but im casting to i32 in the middle so it would lose it anyway
inline i32 ceil(f32 value) {
  if (value - static_cast<f32>(static_cast<i32>(value)) > 0) {
    return static_cast<i32>(value) + 1;
  }
  return static_cast<i32>(value);
}

inline bool f32_eql(f32 a, f32 b) {
  return btl::abs(a - b) <= (constants<f32>::epsilon * btl::max(btl::abs(a), btl::abs(b)));
}

inline f32 radians(f32 deg) {
  return deg * 0.01745329251994329576923690768489f;
}

}

#endif
