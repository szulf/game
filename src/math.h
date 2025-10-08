#ifndef MATH_H
#define MATH_H

inline u64 umin(u64 a, u64 b);
inline i64 imin(i64 a, i64 b);
inline f32 fmin(f32 a, f32 b);
inline u64 umax(u64 a, u64 b);
inline i64 imax(i64 a, i64 b);
inline f32 fmax(f32 a, f32 b);
inline b32 is_power_of_two(usize val);
inline f32 radians(f32 deg);
inline i64 iabs(i64 v);

// TODO(szulf): implement these myself, and stop relying on SDL
inline f32 fsin(f32 rad);
inline f32 fcos(f32 rad);
inline f32 fsqrt(f32 val);
inline f32 fmod(f32 x, f32 y);
inline f32 facos(f32 val);
inline f32 ftan(f32 val);

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

  // TODO(szulf): change this name?
  static Mat4 make(f32 val);
  static Mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far);

  void rotate(f32 rad, const Vec3* axis);
  void translate(const Vec3* vec);

};

#endif
