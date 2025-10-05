#ifndef MATH_H
#define MATH_H

// TODO(szulf): make these inline
static u64 umin(u64 a, u64 b);
static s64 smin(s64 a, s64 b);
static f32 fmin(f32 a, f32 b);
static u64 umax(u64 a, u64 b);
static s64 smax(s64 a, s64 b);
static f32 fmax(f32 a, f32 b);
static b32 is_power_of_two(usize val);
static f32 radians(f32 deg);
static s64 sabs(s64 v);

// TODO(szulf): implement these myself, and stop relying on SDL
static f32 sin(f32 rad);
static f32 cos(f32 rad);
static f32 sqrt(f32 val);
static f32 mod(f32 x, f32 y);
static f32 acos(f32 val);
static f32 tan(f32 val);

typedef struct Vec3
{
  f32 x;
  f32 y;
  f32 z;
} Vec3;

static f32 vec3_length(const Vec3* vec);
static void vec3_normalize(Vec3* vec);

static Vec3 vec3_add(const Vec3* vec1, const Vec3* vec2);
static Vec3 vec3_subtract(const Vec3* vec1, const Vec3* vec2);
static Vec3 vec3_multiply(const Vec3* vec, f32 scalar);
static Vec3 vec3_divide(const Vec3* vec, f32 scalar);

typedef struct Vec4
{
  f32 x;
  f32 y;
  f32 z;
  f32 w;
} Vec4;

static f32 vec4_length(const Vec4* vec);
static void vec4_normalize(Vec4* vec);

static Vec4 vec4_add(const Vec4* vec1, const Vec4* vec2);
static Vec4 vec4_subtract(const Vec4* vec1, const Vec4* vec2);
static Vec4 vec4_multiply(const Vec4* vec, f32 scalar);
static Vec4 vec4_divide(const Vec4* vec, f32 scalar);

typedef struct Mat4
{
  f32 data[16];
} Mat4;

static Mat4 mat4_make(f32 val);
static Vec4 mat4_col(Mat4* mat, usize idx);
static Vec4 mat4_row(Mat4* mat, usize idx);
static void mat4_rotate(Mat4* mat, f32 rad, const Vec3* axis);
static void mat4_translate(Mat4* mat, const Vec3* vec);

// TODO(szulf): change to mat4_perspective?
static Mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far);

#endif
