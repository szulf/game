#include "math.h"

usize pow(usize base, usize exponent)
{
  usize val = 1;
  for (usize i = 0; i < exponent; ++i)
  {
    val *= base;
  }
  return val;
}

i32 abs(i32 value)
{
  if (value < 0)
  {
    return -value;
  }
  return value;
}

f32 max(f32 a, f32 b)
{
  if (a > b)
  {
    return a;
  }
  return b;
}

f32 min(f32 a, f32 b)
{
  if (a > b)
  {
    return b;
  }
  return a;
}

f32 clamp(f32 val, f32 lower, f32 upper)
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

f32 abs(f32 value)
{
  return fabsf(value);
}

// NOTE(szulf): this probably loses some precision, but im casting to i32 in the middle so it would
// lose it anyway
f32 floor(f32 value)
{
  return (f32) (i32) value;
}

// NOTE(szulf): this probably loses some precision, but im casting to i32 in the middle so it would
// lose it anyway
f32 ceil(f32 value)
{
  if (value - (f32) ((i32) value) > 0)
  {
    return (f32) ((i32) value + 1);
  }
  return (f32) (i32) value;
}

f32 round(f32 value)
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

f32 sqrt(f32 value)
{
  return sqrtf(value);
}

f32 sin(f32 value)
{
  return sinf(value);
}

f32 cos(f32 value)
{
  return cosf(value);
}

f32 tan(f32 value)
{
  return tanf(value);
}

f32 asin(f32 value)
{
  return asinf(value);
}

f32 acos(f32 value)
{
  return acosf(value);
}

f32 atan2(f32 y, f32 x)
{
  return atan2f(y, x);
}

f32 wrap_to_neg_pi_to_pi(f32 value)
{
  return atan2(sin(value), cos(value));
}

f32 radians(f32 deg)
{
  return deg * 0.01745329251994329576923690768489f;
}

f32 square(f32 value)
{
  return value * value;
}

bool f32_equal(f32 a, f32 b)
{
  return abs(a - b) <= (F32_EPSILON * max(abs(a), abs(b)));
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

Vec3 operator*(f32 scalar, const Vec3& vec)
{
  return {vec.x * scalar, vec.y * scalar, vec.z * scalar};
}

Vec3 operator*(const Vec3& va, const Vec3& vb)
{
  return {va.x * vb.x, va.y * vb.y, va.z * vb.z};
}

Vec3 operator/(const Vec3& va, f32 scalar)
{
  return {va.x / scalar, va.y / scalar, va.z / scalar};
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

Vec3& operator*=(Vec3& va, const Vec3& vb)
{
  va.x *= vb.x;
  va.y *= vb.y;
  va.z *= vb.z;
  return va;
}

Vec3& operator/=(Vec3& va, f32 scalar)
{
  va.x /= scalar;
  va.y /= scalar;
  va.z /= scalar;
  return va;
}

f32 length(const Vec3& vec)
{
  return sqrt(square(vec.x) + square(vec.y) + square(vec.z));
}

f32 length2(const Vec3& vec)
{
  return square(vec.x) + square(vec.y) + square(vec.z);
}

Vec3 normalize(const Vec3& vec)
{
  auto len = length(vec);
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

Vec3 abs(const Vec3& vec)
{
  return {abs(vec.x), abs(vec.y), abs(vec.z)};
}

f32 dot(const Vec3& va, const Vec3& vb)
{
  return va.x * vb.x + va.y * vb.y + va.z * vb.z;
}

Vec3 cross(const Vec3& va, const Vec3& vb)
{
  return {
    (va.y * vb.z) - (va.z * vb.y),
    (va.z * vb.x) - (va.x * vb.z),
    (va.x * vb.y) - (va.y * vb.x),
  };
}

bool operator==(const Vec3& va, const Vec3& vb)
{
  return f32_equal(va.x, vb.x) && f32_equal(va.y, vb.y) && f32_equal(va.z, vb.z);
}

bool operator!=(const Vec3& va, const Vec3& vb)
{
  return !(va == vb);
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

Mat4 mat4_vertical_perspective(f32 vertical_fov, f32 aspect, f32 near, f32 far)
{
  f32 tangent = tan(vertical_fov / 2.0f);
  f32 top = tangent * near;
  f32 right = top * aspect;
  Mat4 out = {};
  out.data[0] = near / right;
  out.data[5] = near / top;
  out.data[10] = -(far + near) / (far - near);
  out.data[11] = -1;
  out.data[14] = -(2 * near * far) / (far - near);
  out.data[15] = 0;
  return out;
}

Mat4 mat4_orthographic(f32 right, f32 left, f32 top, f32 bottom, f32 near, f32 far)
{
  Mat4 out = {};
  out.data[0] = 2 / (right - left);
  out.data[5] = 2 / (top - bottom);
  out.data[10] = -2 / (far - near);
  out.data[12] = -(right + left) / (right - left);
  out.data[13] = -(top + bottom) / (top - bottom);
  out.data[14] = -(far + near) / (far - near);
  out.data[15] = 1;
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

// TODO(szulf): no idea if this is correct, could be very wrong
void mat4_scale(Mat4& mat, const Vec3& scale)
{
  mat.data[0] *= scale.x;
  mat.data[4] *= scale.x;
  mat.data[8] *= scale.x;

  mat.data[1] *= scale.y;
  mat.data[5] *= scale.y;
  mat.data[9] *= scale.y;

  mat.data[2] *= scale.z;
  mat.data[6] *= scale.z;
  mat.data[10] *= scale.z;
}

void mat4_translate(Mat4& mat, const Vec3& position)
{
  mat.data[12] = position.x;
  mat.data[13] = position.y;
  mat.data[14] = position.z;
}

void mat4_rotate(Mat4& mat, f32 rad, const Vec3& axis)
{
  f32 s = sin(rad);
  f32 c = cos(rad);
  f32 t = 1.0f - c;
  Vec3 u = normalize(axis);

  mat.data[0] = (u.x * u.x) * t + c;
  mat.data[1] = (u.x * u.y) * t - u.z * s;
  mat.data[2] = (u.x * u.z) * t + u.y * s;

  mat.data[4] = (u.x * u.y) * t + u.z * s;
  mat.data[5] = (u.y * u.y) * t + c;
  mat.data[6] = (u.y * u.z) * t - u.x * s;

  mat.data[8] = (u.x * u.z) * t - u.y * s;
  mat.data[9] = (u.y * u.z) * t + u.x * s;
  mat.data[10] = (u.z * u.z) * t + c;
}
