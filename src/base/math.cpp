#include "math.h"

usize usize_pow(usize base, usize exponent)
{
  usize val = 1;
  for (usize i = 0; i < exponent; ++i)
  {
    val *= base;
  }
  return val;
}

i32 i32_abs(i32 value)
{
  return ::abs(value);
}

f32 f32_max(f32 a, f32 b)
{
  if (a > b)
  {
    return a;
  }
  return b;
}

f32 f32_min(f32 a, f32 b)
{
  if (a > b)
  {
    return b;
  }
  return a;
}

f32 f32_clamp(f32 val, f32 lower, f32 upper)
{
  if (val < lower)
  {
    return lower;
  }
  if (val > upper)
  {
    return upper;
  }
  return val;
}

f32 f32_abs(f32 value)
{
  return ::fabsf(value);
}

// NOTE(szulf): this probably loses some precision, but im casting to i32 in the middle so it would lose it anyway
f32 f32_floor(f32 value)
{
  return (f32) (i32) value;
}

// NOTE(szulf): this probably loses some precision, but im casting to i32 in the middle so it would lose it anyway
f32 f32_ceil(f32 value)
{
  if (value - (f32) ((i32) value) > 0)
  {
    return (f32) ((i32) value + 1);
  }
  return (f32) (i32) value;
}

f32 f32_round(f32 value)
{
  auto int_value = (f32) ((i32) value);
  auto t = value - int_value;
  if (t >= 0.5f)
  {
    return int_value + 1.0f;
  }
  else if (t <= -0.5f)
  {
    return int_value - 1.0f;
  }
  return int_value;
}

f32 f32_sqrt(f32 value)
{
  return ::sqrtf(value);
}

f32 f32_sin(f32 value)
{
  return ::sinf(value);
}

f32 f32_cos(f32 value)
{
  return ::cosf(value);
}

f32 f32_tan(f32 value)
{
  return ::tanf(value);
}

f32 f32_radians(f32 deg)
{
  return deg * 0.01745329251994329576923690768489f;
}

bool f32_equal(f32 a, f32 b)
{
  return f32_abs(a - b) <= (F32_EPSILON * f32_max(f32_abs(a), f32_abs(b)));
}

thread_local static u64 random_seed = 98127419834;
u64 u64_random()
{
  random_seed = random_seed * 1664525 + 1013904223;
  return random_seed;
}

bool operator==(const Vec2& va, const Vec2& vb)
{
  return f32_equal(va.x, vb.x) && f32_equal(va.y, vb.y);
}

Vec3 operator-(const Vec3& vec)
{
  return {-vec.x, -vec.y, -vec.z};
}

Vec3 operator+(const Vec3& va, const Vec3& vb)
{
  return {va.x + vb.x, va.y + vb.y, va.z + vb.z};
}

Vec3 operator-(const Vec3& va, const Vec3& vb)
{
  return {va.x - vb.x, va.y - vb.y, va.z - vb.z};
}

Vec3 operator*(const Vec3& vec, f32 scalar)
{
  return {vec.x * scalar, vec.y * scalar, vec.z * scalar};
}

Vec3& operator+=(Vec3& va, const Vec3& vb)
{
  va.x += vb.x;
  va.y += vb.y;
  va.z += vb.z;
  return va;
}

Vec3& operator-=(Vec3& va, const Vec3& vb)
{
  va.x -= vb.x;
  va.y -= vb.y;
  va.z -= vb.z;
  return va;
}

Vec3& operator*=(Vec3& vec, f32 scalar)
{
  vec.x *= scalar;
  vec.y *= scalar;
  vec.z *= scalar;
  return vec;
}

f32 vec3_len(const Vec3& vec)
{
  return f32_sqrt(SQUARE(vec.x) + SQUARE(vec.y) + SQUARE(vec.z));
}

Vec3 vec3_normalize(const Vec3& vec)
{
  auto len = vec3_len(vec);
  if (f32_equal(len, 0))
  {
    return {};
  }
  Vec3 out = {};
  out.x = vec.x / len;
  out.y = vec.y / len;
  out.z = vec.z / len;
  return out;
}

f32 vec3_dot(const Vec3& va, const Vec3& vb)
{
  return va.x * vb.x + va.y * vb.y + va.z * vb.z;
}

Vec3 vec3_cross(const Vec3& va, const Vec3& vb)
{
  return {
    (va.y * vb.z) - (va.z * vb.y),
    (va.z * vb.x) - (va.x * vb.z),
    (va.x * vb.y) - (va.y * vb.x),
  };
}

Vec3 vec3_multiply(const Vec3& va, const Vec3& vb)
{
  return {va.x * vb.x, va.y * vb.y, va.z * vb.z};
}

bool operator==(const Vec3& va, const Vec3& vb)
{
  return f32_equal(va.x, vb.x) && f32_equal(va.y, vb.y) && f32_equal(va.z, vb.z);
}

Mat4 mat4_make()
{
  Mat4 out = {};
  out.data[0] = 1.0f;
  out.data[5] = 1.0f;
  out.data[10] = 1.0f;
  out.data[15] = 1.0f;
  return out;
}

Mat4 mat4_perspective(f32 fov, f32 aspect, f32 near, f32 far)
{
  Mat4 out = {};
  f32 right = near * f32_tan(fov / 2.0f);
  f32 top = right / aspect;
  out.data[0] = near / right;
  out.data[5] = near / top;
  out.data[10] = -(far + near) / (far - near);
  out.data[11] = -1.0f;
  out.data[14] = -(2.0f * far * near) / (far - near);
  out.data[15] = 0.0f;
  return out;
}

void mat4_scale(Mat4& mat, f32 scale)
{
  mat.data[0] *= scale;
  mat.data[4] *= scale;
  mat.data[8] *= scale;

  mat.data[1] *= scale;
  mat.data[5] *= scale;
  mat.data[9] *= scale;

  mat.data[2] *= scale;
  mat.data[6] *= scale;
  mat.data[10] *= scale;
}

void mat4_translate(Mat4& mat, const Vec3& position)
{
  mat.data[12] = position.x;
  mat.data[13] = position.y;
  mat.data[14] = position.z;
}
