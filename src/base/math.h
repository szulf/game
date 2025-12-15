#ifndef MATH_H
#define MATH_H

#include <math.h>
#include <stdlib.h>

#define F32_EPSILON 1.1920928955078125e-07f
#define F32_PI 3.141592653589793238462643383279502884197169399375105820974944592307f

#define U64_FNV_OFFSET 14695981039346656037UL
#define U64_FNV_PRIME 1099511628211UL

usize pow(usize base, usize exponent);

i32 abs(i32 value);

f32 max(f32 a, f32 b);
f32 min(f32 a, f32 b);
f32 clamp(f32 val, f32 a, f32 b);
f32 abs(f32 value);
f32 floor(f32 value);
f32 ceil(f32 value);
f32 round(f32 value);
f32 sqrt(f32 value);
f32 sin(f32 value);
f32 cos(f32 value);
f32 tan(f32 value);
f32 asin(f32 value);
f32 acos(f32 value);
f32 atan2(f32 y, f32 x);
f32 wrap_to_neg_pi_to_pi(f32 value);
f32 radians(f32 deg);
f32 square(f32 value);

bool f32_equal(f32 a, f32 b);

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
Vec3 operator*(const Vec3& vec, f32 scalar);
Vec3 operator*(f32 scalar, const Vec3& vec);
Vec3 operator*(const Vec3& va, const Vec3& vb);
Vec3 operator/(const Vec3& vec, f32 scalar);
Vec3& operator+=(Vec3& va, const Vec3& vb);
Vec3& operator-=(Vec3& va, const Vec3& vb);
Vec3& operator*=(Vec3& va, f32 scalar);
Vec3& operator*=(Vec3& va, const Vec3& vb);
Vec3& operator/=(Vec3& va, f32 scalar);

f32 length(const Vec3& vec);
f32 length2(const Vec3& vec);
Vec3 normalize(const Vec3& vec);
Vec3 abs(const Vec3& vec);
f32 dot(const Vec3& va, const Vec3& vb);
Vec3 cross(const Vec3& va, const Vec3& vb);

bool operator==(const Vec3& va, const Vec3& vb);
bool operator!=(const Vec3& va, const Vec3& vb);

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
