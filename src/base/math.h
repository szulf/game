#ifndef MATH_H
#define MATH_H

#include <math.h>
#include <stdlib.h>

#define F32_EPSILON 1.1920928955078125e-07f
#define F32_PI 3.141592653589793238462643383279502884197169399375105820974944592307f

#define U64_FNV_OFFSET 14695981039346656037UL
#define U64_FNV_PRIME 1099511628211UL

#define SQUARE(x) (x * x)

usize usize_pow(usize base, usize exponent);

i32 i32_abs(i32 value);

f32 f32_max(f32 a, f32 b);
f32 f32_min(f32 a, f32 b);
f32 f32_clamp(f32 val, f32 a, f32 b);
f32 f32_abs(f32 value);
f32 f32_floor(f32 value);
f32 f32_ceil(f32 value);
f32 f32_round(f32 value);
f32 f32_sqrt(f32 value);
f32 f32_sin(f32 value);
f32 f32_cos(f32 value);
f32 f32_tan(f32 value);
f32 f32_radians(f32 deg);

bool f32_equal(f32 a, f32 b);

u64 u64_random();

struct Vec2
{
  f32 x;
  f32 y;
};

bool operator==(const Vec2& va, const Vec2& vb);

struct Vec3
{
  f32 x;
  f32 y;
  f32 z;
};

Vec3 operator-(const Vec3& vec);
Vec3 operator+(const Vec3& va, const Vec3& vb);
Vec3 operator-(const Vec3& va, const Vec3& vb);
Vec3 operator*(const Vec3& va, f32 scalar);
Vec3 operator/(const Vec3& va, f32 scalar);
Vec3& operator+=(Vec3& va, const Vec3& vb);
Vec3& operator-=(Vec3& va, const Vec3& vb);
Vec3& operator*=(const Vec3& va, f32 scalar);
Vec3& operator/=(Vec3& va, f32 scalar);

f32 vec3_len2(const Vec3& vec);
f32 vec3_len(const Vec3& vec);
Vec3 vec3_normalize(const Vec3& vec);
Vec3 vec3_abs(const Vec3& vec);
f32 vec3_dot(const Vec3& va, const Vec3& vb);
Vec3 vec3_cross(const Vec3& va, const Vec3& vb);
Vec3 vec3_multiply(const Vec3& va, const Vec3& vb);

bool operator==(const Vec3& va, const Vec3& vb);

struct Mat4
{
  f32 data[16];
};

Mat4 mat4_make();
Mat4 mat4_perspective(f32 fov, f32 aspect, f32 near, f32 far);

void mat4_scale(Mat4& mat, f32 scale);
void mat4_scale(Mat4& mat, const Vec3& scale);
void mat4_translate(Mat4& mat, const Vec3& position);

#endif
