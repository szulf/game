#include "math.h"

static u64
umin(u64 a, u64 b)
{
  return a < b ? a : b;
}

static s64
smin(s64 a, s64 b)
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

static s64
imax(s64 a, s64 b)
{
  return a > b ? a : b;
}

static f32
fmax(f32 a, f32 b)
{
  return a > b ? a : b;
}

static bool32
is_power_of_two(usize val)
{
  return (val & (val - 1)) == 0;
}

static f32
radians(f32 deg)
{
  return deg * 0.01745329251994329576923690768489f;
}

static f32
vec3_length(const Vec3* vec)
{
  return sqrt((vec->x * vec->x) + (vec->y * vec->y) + (vec->z * vec->z));
}

static void
vec3_normalize(Vec3* vec)
{
  f32 len = vec3_length(vec);
  vec->x = vec->x / len;
  vec->y = vec->y / len;
  vec->z = vec->z / len;
}

static Vec3
vec3_add(const Vec3* vec1, const Vec3* vec2)
{
  return (Vec3) {vec1->x + vec2->x, vec1->y + vec2->y, vec1->z + vec2->z};
}

static Vec3
vec3_subtract(const Vec3* vec1, const Vec3* vec2)
{
  return (Vec3) {vec1->x - vec2->x, vec1->y - vec2->y, vec1->z - vec2->z};
}

static Vec3
vec3_multiply(const Vec3* vec, f32 scalar)
{
  return (Vec3) {vec->x * scalar, vec->y * scalar, vec->z * scalar};
}

static Vec3
vec3_divide(const Vec3* vec, f32 scalar)
{
  return (Vec3) {vec->x / scalar, vec->y / scalar, vec->z / scalar};
}

static f32
vec4_length(const Vec4* vec)
{
  return sqrt((vec->x * vec->x) + (vec->y * vec->y) + (vec->z * vec->z) + (vec->w * vec->w));
}

static void
vec4_normalize(Vec4* vec)
{
  f32 len = vec4_length(vec);
  vec->x = vec->x / len;
  vec->y = vec->y / len;
  vec->z = vec->z / len;
  vec->w = vec->w / len;
}

static Vec4
vec4_add(const Vec4* vec1, const Vec4* vec2)
{
  return (Vec4) {vec1->x + vec2->x, vec1->y + vec2->y, vec1->z + vec2->z, vec1->w + vec2->w};
}

static Vec4
vec4_subtract(const Vec4* vec1, const Vec4* vec2)
{
  return (Vec4) {vec1->x - vec2->x, vec1->y - vec2->y, vec1->z - vec2->z, vec1->w - vec2->w};
}

static Vec4
vec4_multiply(const Vec4* vec, f32 scalar)
{
  return (Vec4) {vec->x * scalar, vec->y * scalar, vec->z * scalar, vec->w * scalar};
}

static Vec4
vec4_divide(const Vec4* vec, f32 scalar)
{
  return (Vec4) {vec->x / scalar, vec->y / scalar, vec->z / scalar, vec->w / scalar};
}

static void
mat4_init(Mat4* mat, f32 val)
{
  mat->data[0] = val;
  mat->data[5] = val;
  mat->data[10] = val;
  mat->data[15] = val;
}

static Vec4
mat4_col(Mat4* mat, usize idx)
{
  ASSERT(idx >= 0 && idx < 4, "index out of bounds");
  // TODO(szulf): is this even correct??
  return (Vec4) {
    mat->data[idx * 4],
    mat->data[idx * 4 + 1],
    mat->data[idx * 4 + 2],
    mat->data[idx * 4 + 3],
  };
}

static void
mat4_rotate(Mat4* mat, f32 rad, const Vec3* axis)
{
  f32 s = sin(rad);
  f32 c = cos(rad);
  f32 t = 1.0f - c;
  Vec3 u = *axis;
  vec3_normalize(&u);

  mat->data[0] = (u.x * u.x) * t + c;
  mat->data[1] = (u.x * u.y) * t - u.z * s;
  mat->data[2] = (u.x * u.z) * t + u.y * s;

  mat->data[4] = (u.x * u.y) * t + u.z * s;
  mat->data[5] = (u.y * u.y) * t + c;
  mat->data[6] = (u.y * u.z) * t - u.x * s;

  mat->data[8] = (u.x * u.z) * t - u.y * s;
  mat->data[9] = (u.y * u.z) * t + u.x * s;
  mat->data[10] = (u.z * u.z) * t + c;
}

static void
mat4_translate(Mat4* mat, const Vec3* vec)
{
  mat->data[12] = vec->x;
  mat->data[13] = vec->y;
  mat->data[14] = vec->z;
}

static Mat4
perspective(f32 fov, f32 aspect, f32 near, f32 far)
{
  f32 right = near * tan(fov / 2.0f);
  f32 top = right / aspect;

  Mat4 mat = {};
  mat.data[0] = near / right;
  mat.data[5] = near / top;
  mat.data[10] = -(far + near) / (far - near);
  mat.data[11] = -1.0f;
  mat.data[14] = -(2.0f * far * near) / (far - near);
  mat.data[15] = 0.0f;
  return mat;
}
