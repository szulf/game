#include "math.h"

#include <math.h>

usize min(usize a, usize b)
{
  if (a > b)
  {
    return b;
  }
  return a;
}

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

// NOTE: this probably loses some precision, but im casting to i32 in the middle so it would
// lose it anyway
f32 floor(f32 value)
{
  return (f32) (i32) value;
}

// NOTE: this probably loses some precision, but im casting to i32 in the middle so it would
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

vec2 operator-(const vec2& vec)
{
  return {-vec.x, -vec.y};
}

vec2 operator+(const vec2& va, const vec2& vb)
{
  return {va.x + vb.x, va.y + vb.y};
}

vec2 operator-(const vec2& va, const vec2& vb)
{
  return {va.x - vb.x, va.y - vb.y};
}

vec2 operator*(const vec2& vec, f32 scalar)
{
  return {vec.x * scalar, vec.y * scalar};
}

vec2 operator*(f32 scalar, const vec2& vec)
{
  return {vec.x * scalar, vec.y * scalar};
}

vec2 operator*(const vec2& va, const vec2& vb)
{
  return {va.x * vb.x, va.y * vb.y};
}

vec2 operator/(const vec2& vec, f32 scalar)
{
  return {vec.x / scalar, vec.y / scalar};
}

vec2& operator+=(vec2& va, const vec2& vb)
{
  va.x += vb.x;
  va.y += vb.y;
  return va;
}

vec2& operator-=(vec2& va, const vec2& vb)
{
  va.x -= vb.x;
  va.y -= vb.y;
  return va;
}

vec2& operator*=(vec2& va, f32 scalar)
{
  va.x *= scalar;
  va.y *= scalar;
  return va;
}

vec2& operator*=(vec2& va, const vec2& vb)
{
  va.x *= vb.x;
  va.y *= vb.y;
  return va;
}

vec2& operator/=(vec2& va, f32 scalar)
{
  va.x /= scalar;
  va.y /= scalar;
  return va;
}

bool operator==(const vec2& va, const vec2& vb)
{
  return f32_equal(va.x, vb.x) && f32_equal(va.y, vb.y);
}

vec3 operator-(const vec3& vec)
{
  return {-vec.x, -vec.y, -vec.z};
}

vec3 operator+(const vec3& va, const vec3& vb)
{
  return {va.x + vb.x, va.y + vb.y, va.z + vb.z};
}

vec3 operator-(const vec3& va, const vec3& vb)
{
  return {va.x - vb.x, va.y - vb.y, va.z - vb.z};
}

vec3 operator*(const vec3& vec, f32 scalar)
{
  return {vec.x * scalar, vec.y * scalar, vec.z * scalar};
}

vec3 operator*(f32 scalar, const vec3& vec)
{
  return {vec.x * scalar, vec.y * scalar, vec.z * scalar};
}

vec3 operator*(const vec3& va, const vec3& vb)
{
  return {va.x * vb.x, va.y * vb.y, va.z * vb.z};
}

vec3 operator/(const vec3& va, f32 scalar)
{
  return {va.x / scalar, va.y / scalar, va.z / scalar};
}

vec3& operator+=(vec3& va, const vec3& vb)
{
  va.x += vb.x;
  va.y += vb.y;
  va.z += vb.z;
  return va;
}

vec3& operator-=(vec3& va, const vec3& vb)
{
  va.x -= vb.x;
  va.y -= vb.y;
  va.z -= vb.z;
  return va;
}

vec3& operator*=(vec3& vec, f32 scalar)
{
  vec.x *= scalar;
  vec.y *= scalar;
  vec.z *= scalar;
  return vec;
}

vec3& operator*=(vec3& va, const vec3& vb)
{
  va.x *= vb.x;
  va.y *= vb.y;
  va.z *= vb.z;
  return va;
}

vec3& operator/=(vec3& va, f32 scalar)
{
  va.x /= scalar;
  va.y /= scalar;
  va.z /= scalar;
  return va;
}

f32 length(const vec3& vec)
{
  return sqrt(square(vec.x) + square(vec.y) + square(vec.z));
}

f32 length2(const vec3& vec)
{
  return square(vec.x) + square(vec.y) + square(vec.z);
}

vec3 normalize(const vec3& vec)
{
  auto len = length(vec);
  if (f32_equal(len, 0))
  {
    return {};
  }
  vec3 out = {};
  out.x = vec.x / len;
  out.y = vec.y / len;
  out.z = vec.z / len;
  return out;
}

vec3 abs(const vec3& vec)
{
  return {abs(vec.x), abs(vec.y), abs(vec.z)};
}

f32 dot(const vec3& va, const vec3& vb)
{
  return va.x * vb.x + va.y * vb.y + va.z * vb.z;
}

vec3 cross(const vec3& va, const vec3& vb)
{
  return {
    (va.y * vb.z) - (va.z * vb.y),
    (va.z * vb.x) - (va.x * vb.z),
    (va.x * vb.y) - (va.y * vb.x),
  };
}

bool operator==(const vec3& va, const vec3& vb)
{
  return f32_equal(va.x, vb.x) && f32_equal(va.y, vb.y) && f32_equal(va.z, vb.z);
}

bool operator!=(const vec3& va, const vec3& vb)
{
  return !(va == vb);
}

mat4 mat4::make()
{
  mat4 out = {};
  out.data[0][0] = 1.0f;
  out.data[1][1] = 1.0f;
  out.data[2][2] = 1.0f;
  out.data[3][3] = 1.0f;
  return out;
}

mat4 mat4::perspective(f32 fov, f32 aspect, f32 near, f32 far, bool vertical)
{
  f32 tangent = tan(fov / 2.0f);
  f32 top;
  f32 right;
  if (vertical)
  {
    top = tangent * near;
    right = top * aspect;
  }
  else
  {
    right = tangent * near;
    top = right / aspect;
  }
  mat4 out = {};
  out.data[0][0] = near / right;
  out.data[1][1] = near / top;
  out.data[2][2] = -(far + near) / (far - near);
  out.data[2][3] = -1;
  out.data[3][2] = -(2 * near * far) / (far - near);
  out.data[3][3] = 0;
  return out;
}

mat4 mat4::orthographic(f32 right, f32 left, f32 top, f32 bottom, f32 near, f32 far)
{
  mat4 out = {};
  out.data[0][0] = 2 / (right - left);
  out.data[1][1] = 2 / (top - bottom);
  out.data[2][2] = -2 / (far - near);
  out.data[3][0] = -(right + left) / (right - left);
  out.data[3][1] = -(top + bottom) / (top - bottom);
  out.data[3][2] = -(far + near) / (far - near);
  out.data[3][3] = 1;
  return out;
}

mat4 mat4::look_at(const vec3& pos, const vec3& target, const vec3& up)
{
  vec3 f = normalize(target - pos);
  vec3 s = normalize(cross(f, up));
  vec3 u = cross(s, f);

  mat4 out = {};

  out.data[0][0] = s.x;
  out.data[1][0] = s.y;
  out.data[2][0] = s.z;
  out.data[3][0] = -dot(s, pos);

  out.data[0][1] = u.x;
  out.data[1][1] = u.y;
  out.data[2][1] = u.z;
  out.data[3][1] = -dot(u, pos);

  out.data[0][2] = -f.x;
  out.data[1][2] = -f.y;
  out.data[2][2] = -f.z;
  out.data[3][2] = dot(f, pos);

  out.data[0][3] = 0.0f;
  out.data[1][3] = 0.0f;
  out.data[2][3] = 0.0f;
  out.data[3][3] = 1.0f;

  return out;
}

mat4 operator*(const mat4& ma, const mat4& mb)
{
  mat4 out = {};
  for (usize i = 0; i < 4; ++i)
  {
    for (usize j = 0; j < 4; ++j)
    {
      for (usize k = 0; k < 4; ++k)
      {
        out.data[j][i] += ma.data[k][i] * mb.data[j][k];
      }
    }
  }
  return out;
}

void scale(mat4& mat, f32 scale)
{
  mat.data[0][0] *= scale;
  mat.data[1][0] *= scale;
  mat.data[2][0] *= scale;

  mat.data[0][1] *= scale;
  mat.data[1][1] *= scale;
  mat.data[2][1] *= scale;

  mat.data[0][2] *= scale;
  mat.data[1][2] *= scale;
  mat.data[2][2] *= scale;
}

void scale(mat4& mat, const vec3& scale)
{
  mat.data[0][0] *= scale.x;
  mat.data[0][1] *= scale.x;
  mat.data[0][2] *= scale.x;

  mat.data[1][0] *= scale.y;
  mat.data[1][1] *= scale.y;
  mat.data[1][2] *= scale.y;

  mat.data[2][0] *= scale.z;
  mat.data[2][1] *= scale.z;
  mat.data[2][2] *= scale.z;
}

void translate(mat4& mat, const vec3& position)
{
  mat.data[3][0] = position.x;
  mat.data[3][1] = position.y;
  mat.data[3][2] = position.z;
}

void rotate(mat4& mat, f32 rad, const vec3& axis)
{
  f32 s = sin(rad);
  f32 c = cos(rad);
  f32 t = 1.0f - c;
  vec3 u = normalize(axis);

  mat.data[0][0] = (u.x * u.x) * t + c;
  mat.data[0][1] = (u.x * u.y) * t - u.z * s;
  mat.data[0][2] = (u.x * u.z) * t + u.y * s;

  mat.data[1][0] = (u.x * u.y) * t + u.z * s;
  mat.data[1][1] = (u.y * u.y) * t + c;
  mat.data[1][2] = (u.y * u.z) * t - u.x * s;

  mat.data[2][0] = (u.x * u.z) * t - u.y * s;
  mat.data[2][1] = (u.y * u.z) * t + u.x * s;
  mat.data[2][2] = (u.z * u.z) * t + c;
}
