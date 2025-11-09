#ifndef MATH_H
#define MATH_H

namespace math
{

// TODO(szulf): make these inline
static auto is_power_of_two(usize val) -> bool;
static auto radians(f32 deg) -> f32;

struct vec2
{
  f32 x;
  f32 y;

  auto operator==(const vec2& other) const -> bool;
};

struct vec3
{
  f32 x{};
  f32 y{};
  f32 z{};

  vec3() {}
  vec3(f32 v) : x{v}, y{v}, z{v} {}
  vec3(f32 x, f32 y, f32 z) : x{x}, y{y}, z{z} {}

  auto length() const -> f32;
  auto normalize() -> void;

  auto operator==(const vec3& other) const -> bool;
  auto operator+(const vec3& other) const -> vec3;
  auto operator-(const vec3& other) const -> vec3;
  auto multiply(f32 scalar) const -> vec3;
  auto divide(f32 scalar) const -> vec3;
};

struct vec4
{
  f32 x;
  f32 y;
  f32 z;
  f32 w;

  auto length() const -> f32;
  auto normalize() -> void;

  auto operator+(const vec4& other) const -> vec4;
  auto operator-(const vec4& other) const -> vec4;
  auto multiply(f32 scalar) const -> vec4;
  auto divide(f32 scalar) const -> vec4;
};

struct quat
{
  f32 r{1.0f};
  f32 i{};
  f32 j{};
  f32 k{};

  quat() {}
  quat(f32 degree, vec3 axis);

  auto length() const -> f32;
  auto normalize() -> void;
  auto get_half_angle() const -> f32;
  static auto slerp(const quat& q1, const quat& q2, f32 t) -> quat;

  auto operator+(const quat& other) const -> quat;
  auto operator*(f32 scalar) const -> quat;
  auto operator/(f32 scalar) const -> quat;
};

struct mat4
{
  f32 data[16]{};

  mat4(f32 val);
  mat4(const quat& rotation, const vec3& pos, const vec3& scale_vec);
  static auto perspective(f32 fov, f32 aspect, f32 near, f32 far) -> mat4;

  auto scale(const vec3& vec) -> void;
  auto rotate(const quat& q) -> void;
  auto translate(const vec3& vec) -> void;
};

}

#endif
