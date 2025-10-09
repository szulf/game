#include "math.h"

inline bool
is_power_of_two(usize val)
{
  return (val & (val - 1)) == 0;
}

inline f32
radians(f32 deg)
{
  return deg * 0.01745329251994329576923690768489f;
}

bool
Vec2::operator==(const Vec2& other) const
{
  return x == other.x && y == other.y;
}

bool Vec2::operator!=(const Vec2& other) const
{
  return !(*this == other);
}

f32
Vec3::length() const
{
  return std::sqrt((x * x) + (y * y) + (z * z));
}

void
Vec3::normalize()
{
  f32 len = length();
  x = x / len;
  y = y / len;
  z = z / len;
}

Vec3
Vec3::operator+(const Vec3& other) const
{
  return {x + other.x, y + other.y, z + other.z};
}

Vec3
Vec3::operator-(const Vec3& other) const
{
  return {x - other.x, y - other.y, z - other.z};
}

bool
Vec3::operator==(const Vec3& other) const
{
  return x == other.x && y == other.y && z == other.z;
}

bool
Vec3::operator!=(const Vec3& other) const
{
  return !(*this == other);
}

Vec3
Vec3::multiply(f32 scalar) const
{
  return {x * scalar, y * scalar, z * scalar};
}

Vec3
Vec3::divide(f32 scalar) const
{
  return {x / scalar, y / scalar, z / scalar};
}

f32
Vec4::length() const
{
  return fsqrt((x * x) + (y * y) + (z * z) + (w * w));
}

void
Vec4::normalize()
{
  f32 len = length();
  x = x / len;
  y = y / len;
  z = z / len;
  w = w / len;
}

Vec4
Vec4::operator+(const Vec4& other) const
{
  return {x + other.x, y + other.y, z + other.z, w + other.w};
}

Vec4
Vec4::operator-(const Vec4& other) const
{
  return {x - other.x, y - other.y, z - other.z, w - other.w};
}

bool
Vec4::operator==(const Vec4& other) const
{
  return x == other.x && y == other.y && z == other.z && w == other.w;
}

bool
Vec4::operator!=(const Vec4& other) const
{
  return !(*this == other);
}

Vec4
Vec4::multiply(f32 scalar) const
{
  return {x * scalar, y * scalar, z * scalar, w * scalar};
}

Vec4
Vec4::divide(f32 scalar) const
{
  return {x / scalar, y / scalar, z / scalar, w / scalar};
}

Mat4::Mat4(f32 val)
{
  data[0] = val;
  data[5] = val;
  data[10] = val;
  data[15] = val;
}

Mat4
Mat4::perspective(f32 fov, f32 aspect, f32 near, f32 far)
{
  f32 right = near * std::tan(fov / 2.0f);
  f32 top = right / aspect;

  Mat4 mat{};
  mat.data[0] = near / right;
  mat.data[5] = near / top;
  mat.data[10] = -(far + near) / (far - near);
  mat.data[11] = -1.0f;
  mat.data[14] = -(2.0f * far * near) / (far - near);
  mat.data[15] = 0.0f;
  return mat;
}

void
Mat4::rotate(f32 rad, const Vec3* axis)
{
  f32 s = std::sin(rad);
  f32 c = std::cos(rad);
  f32 t = 1.0f - c;
  Vec3 u = *axis;
  u.normalize();

  data[0] = (u.x * u.x) * t + c;
  data[1] = (u.x * u.y) * t - u.z * s;
  data[2] = (u.x * u.z) * t + u.y * s;

  data[4] = (u.x * u.y) * t + u.z * s;
  data[5] = (u.y * u.y) * t + c;
  data[6] = (u.y * u.z) * t - u.x * s;

  data[8] = (u.x * u.z) * t - u.y * s;
  data[9] = (u.y * u.z) * t + u.x * s;
  data[10] = (u.z * u.z) * t + c;
}

void
Mat4::translate(const Vec3* vec)
{
  data[12] = vec->x;
  data[13] = vec->y;
  data[14] = vec->z;
}
