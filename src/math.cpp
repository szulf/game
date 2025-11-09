#include "math.h"

namespace math
{

static auto is_power_of_two(usize val) -> bool
{
  return (val & (val - 1)) == 0;
}

static auto radians(f32 deg) -> f32
{
  return deg * 0.01745329251994329576923690768489f;
}

auto vec2::operator==(const vec2& other) const -> bool
{
  return x == other.x && y == other.y;
}

auto vec3::length() const -> f32
{
  return std::sqrt((x * x) + (y * y) + (z * z));
}

auto vec3::normalize() -> void
{
  f32 len = length();
  x = x / len;
  y = y / len;
  z = z / len;
}

auto vec3::operator==(const vec3& other) const -> bool
{
  return x == other.x && y == other.y && z == other.z;
}

auto vec3::operator+(const vec3& other) const -> vec3
{
  return vec3{x + other.x, y + other.y, z + other.z};
}

auto vec3::operator-(const vec3& other) const -> vec3
{
  return vec3{x - other.x, y - other.y, z - other.z};
}

auto vec3::multiply(f32 scalar) const -> vec3
{
  return vec3{x * scalar, y * scalar, z * scalar};
}

auto vec3::divide(f32 scalar) const -> vec3
{
  return vec3{x / scalar, y / scalar, z / scalar};
}

auto vec4::length() const -> f32
{
  return std::sqrt((x * x) + (y * y) + (z * z) + (w * w));
}

auto vec4::normalize() -> void
{
  f32 len = length();
  x = x / len;
  y = y / len;
  z = z / len;
  w = w / len;
}

auto vec4::operator+(const vec4& other) const -> vec4
{
  return vec4{x + other.x, y + other.y, z + other.z, w + other.w};
}

auto vec4::operator-(const vec4& other) const -> vec4
{
  return vec4{x - other.x, y - other.y, z - other.z, w - other.w};
}

auto vec4::multiply(f32 scalar) const -> vec4
{
  return vec4{x * scalar, y * scalar, z * scalar, w * scalar};
}

auto vec4::divide(f32 scalar) const -> vec4
{
  return vec4{x / scalar, y / scalar, z / scalar, w / scalar};
}

quat::quat(f32 degree, vec3 axis)
{
  f32 rad = radians(degree);
  f32 s = std::sin(rad / 2.0f);
  f32 c = std::cos(rad / 2.0f);
  axis.normalize();

  r = c;
  i = axis.x * s;
  j = axis.y * s;
  k = axis.z * s;
}

auto quat::length() const -> f32
{
  return std::sqrt(r * r + i * i + j * j + k * k);
}

auto quat::normalize() -> void
{
  f32 len = length();
  r /= len;
  i /= len;
  j /= len;
  k /= len;
}

auto quat::get_half_angle() const -> f32
{
  return std::atan2(std::sqrt(i * i + j * j + k * k), r);
}

auto quat::slerp(const quat& q1, const quat& q2, f32 t) -> quat
{
  f32 theta = std::abs(q1.get_half_angle() - q2.get_half_angle());
  return (q1 * std::sin((1 - t) * theta) + q2 * std::sin(t * theta)) / std::sin(theta);
}

auto quat::operator+(const quat& other) const -> quat
{
  quat out{};
  out.r = r + other.r;
  out.i = i + other.i;
  out.j = j + other.j;
  out.k = k + other.k;
  return out;
}

auto quat::operator*(f32 scalar) const -> quat
{
  quat out{*this};
  out.r *= scalar;
  out.i *= scalar;
  out.j *= scalar;
  out.k *= scalar;
  return out;
}

auto quat::operator/(f32 scalar) const -> quat
{
  quat out{*this};
  out.r /= scalar;
  out.i /= scalar;
  out.j /= scalar;
  out.k /= scalar;
  return out;
}

mat4::mat4(f32 val)
{
  data[0] = val;
  data[5] = val;
  data[10] = val;
  data[15] = val;
}

mat4::mat4(const quat& rotation, const vec3& pos, const vec3& scale_vec)
{
  *this = mat4{1.0f};
  translate(pos);
  rotate(rotation);
  scale(scale_vec);
}

auto mat4::perspective(f32 fov, f32 aspect, f32 near, f32 far) -> mat4
{
  f32 right = near * std::tan(fov / 2.0f);
  f32 top = right / aspect;

  mat4 mat = {0};
  mat.data[0] = near / right;
  mat.data[5] = near / top;
  mat.data[10] = -(far + near) / (far - near);
  mat.data[11] = -1.0f;
  mat.data[14] = -(2.0f * far * near) / (far - near);
  mat.data[15] = 0.0f;
  return mat;
}

auto mat4::scale(const vec3& vec) -> void
{
  data[0] *= vec.x;
  data[1] *= vec.x;
  data[2] *= vec.x;
  data[3] *= vec.x;

  data[4] *= vec.y;
  data[5] *= vec.y;
  data[6] *= vec.y;
  data[7] *= vec.y;

  data[8] *= vec.z;
  data[9] *= vec.z;
  data[10] *= vec.z;
  data[11] *= vec.z;
}

auto mat4::rotate(const quat& q) -> void
{
  data[0] = 1 - (2 * q.j * q.j) - (2 * q.k * q.k);
  data[1] = (2 * q.i * q.j) - (2 * q.k * q.r);
  data[2] = (2 * q.i * q.k) + (2 * q.j * q.r);

  data[4] = (2 * q.i * q.j) + (2 * q.k * q.r);
  data[5] = 1 - (2 * q.i * q.i) - (2 * q.k * q.k);
  data[6] = (2 * q.j * q.k) - (2 * q.i * q.r);

  data[8] = (2 * q.i * q.k) - (2 * q.j * q.r);
  data[9] = (2 * q.j * q.k) + (2 * q.i * q.r);
  data[10] = 1 - (2 * q.i * q.i) - (2 * q.j * q.j);
}

auto mat4::translate(const vec3& vec) -> void
{
  data[12] = vec.x;
  data[13] = vec.y;
  data[14] = vec.z;
}
}

template <>
struct std::formatter<math::vec2>
{
  template <class T>
  constexpr auto parse(T& ctx) -> T::iterator
  {
    return ctx.begin();
  }

  template <class T>
  auto format(const math::vec2& v, T& ctx) const -> T::iterator
  {
    return std::format_to(ctx.out(), "vec2{{{}, {}}}", v.x, v.y);
  }
};

template <>
struct std::formatter<math::vec3>
{
  template <class T>
  constexpr auto parse(T& ctx) -> T::iterator
  {
    return ctx.begin();
  }

  template <class T>
  auto format(const math::vec3& v, T& ctx) const -> T::iterator
  {
    return std::format_to(ctx.out(), "vec3{{{}, {}, {}}}", v.x, v.y, v.z);
  }
};

template <>
struct std::formatter<math::quat>
{
  template <class T>
  constexpr auto parse(T& ctx) -> T::iterator
  {
    return ctx.begin();
  }

  template <class T>
  auto format(const math::quat& v, T& ctx) const -> T::iterator
  {
    return std::format_to(ctx.out(), "quat{{{} {}i {}j {}k}}", v.r, v.i, v.j, v.k);
  }
};

template <>
struct std::formatter<math::mat4>
{
  template <class T>
  constexpr auto parse(T& ctx) -> T::iterator
  {
    return ctx.begin();
  }

  template <class T>
  auto format(const math::mat4& v, T& ctx) const -> T::iterator
  {
    return std::format_to(
      ctx.out(),
      "mat4{{{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}}}",
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
