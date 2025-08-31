#ifndef VEC4_H
#define VEC4_H

struct Vec4
{
  union
  {
    struct
    {
      f32 x;
      f32 y;
      f32 z;
      f32 w;
    };
    struct
    {
      f32 r;
      f32 g;
      f32 b;
      f32 a;
    };
    f32 data[4];
  };

  Vec4(f32 x, f32 y, f32 z, f32 w) : data{x, y, z, w} {}

  f32 length();
  void normalize();

  Vec4& operator+=(const Vec4& other);
  Vec4& operator-=(const Vec4& other);
  Vec4& operator*=(const f32& other);
  Vec4& operator/=(const f32& other);

  Vec4 operator+(const Vec4& other);
  Vec4 operator-(const Vec4& other);
  Vec4 operator*(const f32& other);
  Vec4 operator/(const f32& other);

  f32& operator[](usize idx);
  const f32& operator[](usize idx) const;
};

#endif
