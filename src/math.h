#ifndef MATH_H
#define MATH_H

inline f32 radians(f32 deg);

struct Vec2 {
  f32 x{};
  f32 y{};

  bool operator==(const Vec2& other) const;
  bool operator!=(const Vec2& other) const;
};

struct Vec3 {
  f32 x{};
  f32 y{};
  f32 z{};

  Vec3() {}
  Vec3(f32 x, f32 y, f32 z) : x{x}, y{y}, z{z} {}

  f32 length() const;
  void normalize();

  Vec3 operator+(const Vec3& other) const;
  Vec3 operator+=(const Vec3& other);
  Vec3 operator-(const Vec3& other) const;
  Vec3 operator-=(const Vec3& other);

  bool operator==(const Vec3& other) const;
  bool operator!=(const Vec3& other) const;

  Vec3 multiply(f32 scalar) const;
  Vec3 divide(f32 scalar) const;
};

struct Vec4 {
  f32 x{};
  f32 y{};
  f32 z{};
  f32 w{};

  Vec4() {}
  Vec4(f32 x, f32 y, f32 z, f32 w) : x{x}, y{y}, z{z}, w{w} {}

  f32 length() const;
  void normalize();

  Vec4 operator+(const Vec4& other) const;
  Vec4 operator+=(const Vec4& other);
  Vec4 operator-(const Vec4& other) const;
  Vec4 operator-=(const Vec4& other);

  bool operator==(const Vec4& other) const;
  bool operator!=(const Vec4& other) const;

  Vec4 multiply(f32 scalar) const;
  Vec4 divide(f32 scalar) const;
};

struct Mat4 {
  f32 data[16]{};

  Mat4() {}
  Mat4(f32 val);
  static Mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far);

  void rotate(f32 rad, const Vec3* axis);
  void translate(const Vec3* vec);

};

#endif
