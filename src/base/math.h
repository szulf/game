#ifndef MATH_H
#define MATH_H

#include <math.h>
#include <stdlib.h>

#define F32_EPSILON 1.1920928955078125e-07f
#define F32_PI 3.141592653589793238462643383279502884197169399375105820974944592307f

#define F32_G 9.81f

#define U64_FNV_OFFSET 14695981039346656037UL
#define U64_FNV_PRIME 1099511628211UL

usize min(usize a, usize b);
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

struct vec2
{
  f32 x;
  f32 y;
};

bool operator==(const vec2& va, const vec2& vb);

struct vec3
{
  f32 x;
  f32 y;
  f32 z;
};

vec3 operator-(const vec3& vec);
vec3 operator+(const vec3& va, const vec3& vb);
vec3 operator-(const vec3& va, const vec3& vb);
vec3 operator*(const vec3& vec, f32 scalar);
vec3 operator*(f32 scalar, const vec3& vec);
vec3 operator*(const vec3& va, const vec3& vb);
vec3 operator/(const vec3& vec, f32 scalar);
vec3& operator+=(vec3& va, const vec3& vb);
vec3& operator-=(vec3& va, const vec3& vb);
vec3& operator*=(vec3& va, f32 scalar);
vec3& operator*=(vec3& va, const vec3& vb);
vec3& operator/=(vec3& va, f32 scalar);

f32 length(const vec3& vec);
f32 length2(const vec3& vec);
vec3 normalize(const vec3& vec);
vec3 abs(const vec3& vec);
f32 dot(const vec3& va, const vec3& vb);
vec3 cross(const vec3& va, const vec3& vb);

bool operator==(const vec3& va, const vec3& vb);
bool operator!=(const vec3& va, const vec3& vb);

struct vec4
{
  f32 x;
  f32 y;
  f32 z;
  f32 w;
};

// NOTE: column major
struct mat4
{
  union
  {
    f32 data[4][4];
    f32 raw_data[16];
  };

  static mat4 make();
  static mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far, bool vertical);
  static mat4 orthographic(f32 right, f32 left, f32 top, f32 bottom, f32 near, f32 far);
  static mat4 look_at(const vec3& pos, const vec3& target, const vec3& up);
};

mat4 operator*(const mat4& ma, const mat4& mb);

void scale(mat4& mat, f32 scale);
void scale(mat4& mat, const vec3& scale);
void translate(mat4& mat, const vec3& position);
void rotate(mat4& mat, f32 rad, const vec3& axis);

#endif
