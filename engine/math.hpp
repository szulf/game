#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <format>

namespace math {

inline bool float_eql(float a, float b) {
  return std::abs(a - b) <= (std::numeric_limits<float>::epsilon() * std::max(std::abs(a), std::abs(b)));
}

inline float radians(float deg) {
  return deg * 0.01745329251994329576923690768489f;
}

struct vec2 final {
  float x{};
  float y{};

  constexpr bool operator==(const vec2& other) const noexcept {
    return float_eql(x, other.x) && float_eql(y, other.y);
  }
};

struct vec3 final {
  float x{};
  float y{};
  float z{};

  [[nodiscard]] constexpr float length() const noexcept {
    return std::sqrt(x * x + y * y + z * z);
  }
  [[nodiscard]] constexpr vec3 normalize() const noexcept {
    vec3 out;
    auto len = length();
    out.x = x / len;
    out.y = y / len;
    out.z = z / len;
    return out;
  }
  [[nodiscard]] constexpr bool operator==(const vec3& other) const noexcept {
    return float_eql(x, other.x) && float_eql(y, other.y) && float_eql(z, other.z);
  }

  [[nodiscard]] constexpr vec3 operator-() const noexcept {
    return vec3{-x, -y, -z};
  }
};

[[nodiscard]] vec3 cross(const vec3& va, const vec3& vb) noexcept;

struct mat4 final {
  constexpr mat4() noexcept {
    data[0] = 1.0f;
    data[5] = 1.0f;
    data[10] = 1.0f;
    data[15] = 1.0f;
  }
  constexpr mat4(float fov, float aspect, float near, float far) noexcept {
    float right = near * std::tan(fov / 2.0f);
    float top = right / aspect;
    data[0] = near / right;
    data[5] = near / top;
    data[10] = -(far + near) / (far - near);
    data[11] = -1.0f;
    data[14] = -(2.0f * far * near) / (far - near);
    data[15] = 0.0f;
  }

  constexpr void translate(const math::vec3& vec) noexcept {
    data[12] = vec.x;
    data[13] = vec.y;
    data[14] = vec.z;
  }

  void rotate(float rad, const vec3& axis) noexcept;

  float data[16]{};
};

}

template <>
struct std::formatter<math::vec2> {
  template <class T>
  constexpr T::iterator parse(T& ctx) {
    return ctx.begin();
  }

  template <class T>
  T::iterator format(const math::vec2& v, T& ctx) const {
    return std::format_to(ctx.out(), "vec2{{{} {}}}", v.x, v.y);
  }
};

template <>
struct std::formatter<math::vec3> {
  template <class T>
  constexpr T::iterator parse(T& ctx) {
    return ctx.begin();
  }

  template <class T>
  T::iterator format(const math::vec3& v, T& ctx) const {
    return std::format_to(ctx.out(), "vec3{{{} {} {}}}", v.x, v.y, v.z);
  }
};

template <>
struct std::formatter<math::mat4> {
  template <class T>
  constexpr T::iterator parse(T& ctx) {
    return ctx.begin();
  }

  template <class T>
  T::iterator format(const math::mat4& v, T& ctx) const {
    return std::format_to(
      ctx.out(),
      "mat4{{{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}}}\n",
      v.data[0],
      v.data[1],
      v.data[2],
      v.data[3],
      v.data[4],
      v.data[5],
      v.data[6],
      v.data[7],
      v.data[8],
      v.data[9],
      v.data[10],
      v.data[11],
      v.data[12],
      v.data[13],
      v.data[14],
      v.data[15]
    );
  }
};
