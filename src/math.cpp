#include "math.h"

static u64
umin(u64 a, u64 b)
{
  return a < b ? a : b;
}

static i64
imin(i64 a, i64 b)
{
  return a < b ? a : b;
}

static f32
fmin(f32 a, f32 b)
{
  return a < b ? a : b;
}

static u64
umax(u64 a, u64 b)
{
  return a > b ? a : b;
}

static i64
imax(i64 a, i64 b)
{
  return a > b ? a : b;
}

static f32
fmax(f32 a, f32 b)
{
  return a > b ? a : b;
}

static b32
is_power_of_two(usize val)
{
  return (val & (val - 1)) == 0;
}

static f32
radians(f32 deg)
{
  return deg * 0.01745329251994329576923690768489f;
}

static i64
iabs(i64 v)
{
  return v * ((v > 0) - (v < 0));
}

static u64
upow(u64 a, u64 b)
{
  u64 p = 1;
  while (b--)
  {
    p *= a;
  }
  return p;
}

f32
Vec3::length() const
{
  return sqrt((x * x) + (y * y) + (z * z));
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
  return Vec3{x + other.x, y + other.y, z + other.z};
}

Vec3
Vec3::operator-(const Vec3& other) const
{
  return Vec3{x - other.x, y - other.y, z - other.z};
}

Vec3
Vec3::multiply(f32 scalar) const
{
  return Vec3{x * scalar, y * scalar, z * scalar};
}

Vec3
Vec3::divide(f32 scalar) const
{
  return Vec3{x / scalar, y / scalar, z / scalar};
}

f32
Vec4::length() const
{
  return sqrt((x * x) + (y * y) + (z * z) + (w * w));
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
  return Vec4{x + other.x, y + other.y, z + other.z, w + other.w};
}

Vec4
Vec4::operator-(const Vec4& other) const
{
  return Vec4{x - other.x, y - other.y, z - other.z, w - other.w};
}

Vec4
Vec4::multiply(f32 scalar) const
{
  return Vec4{x * scalar, y * scalar, z * scalar, w * scalar};
}

Vec4
Vec4::divide(f32 scalar) const
{
  return Vec4{x / scalar, y / scalar, z / scalar, w / scalar};
}

Mat4
Mat4::make(f32 val)
{
  Mat4 mat = {0};
  mat.data[0] = val;
  mat.data[5] = val;
  mat.data[10] = val;
  mat.data[15] = val;
  return mat;
}

Mat4
Mat4::perspective(f32 fov, f32 aspect, f32 near, f32 far)
{
  f32 right = near * tan(fov / 2.0f);
  f32 top = right / aspect;

  Mat4 mat = {0};
  mat.data[0] = near / right;
  mat.data[5] = near / top;
  mat.data[10] = -(far + near) / (far - near);
  mat.data[11] = -1.0f;
  mat.data[14] = -(2.0f * far * near) / (far - near);
  mat.data[15] = 0.0f;
  return mat;
}

void
Mat4::rotate(f32 rad, Vec3 u)
{
  f32 s = sin(rad);
  f32 c = cos(rad);
  f32 t = 1.0f - c;
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
Mat4::translate(const Vec3& vec)
{
  data[12] = vec.x;
  data[13] = vec.y;
  data[14] = vec.z;
}

