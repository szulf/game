#ifndef MATH_H
#define MATH_H

// TODO(szulf): make these inline
static u64 umin(u64 a, u64 b);
static i64 imin(i64 a, i64 b);
static f32 fmin(f32 a, f32 b);
static u64 umax(u64 a, u64 b);
static i64 imax(i64 a, i64 b);
static f32 fmax(f32 a, f32 b);
static b32 is_power_of_two(usize val);
static f32 radians(f32 deg);
static i64 iabs(i64 v);
static u64 upow(u64 a, u64 b);

// TODO(szulf): implement these myself, and stop relying on SDL
static f32 sin(f32 rad);
static f32 cos(f32 rad);
static f32 sqrt(f32 val);
static f32 mod(f32 x, f32 y);
static f32 acos(f32 val);
static f32 tan(f32 val);

struct Vec2
{
  f32 x;
  f32 y;
};

struct Vec3
{
  f32 x;
  f32 y;
  f32 z;

  f32 length() const;
  void normalize();

  Vec3 operator+(const Vec3& other) const;
  Vec3 operator-(const Vec3& other) const;
  Vec3 multiply(f32 scalar) const;
  Vec3 divide(f32 scalar) const;
};

struct Vec4
{
  f32 x;
  f32 y;
  f32 z;
  f32 w;

  f32 length() const;
  void normalize();

  Vec4 operator+(const Vec4& other) const;
  Vec4 operator-(const Vec4& other) const;
  Vec4 multiply(f32 scalar) const;
  Vec4 divide(f32 scalar) const;
};

struct Mat4
{
  f32 data[16];

  static Mat4 make(f32 val);
  static Mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far);

  void rotate(f32 rad, Vec3 axis);
  void translate(const Vec3& vec);
};

#endif
