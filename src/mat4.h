#ifndef MAT4_H
#define MAT4_H

namespace math
{

// TODO(szulf): move this later
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
    f32 components[4];
  };

  Vec4(f32 x, f32 y, f32 z, f32 w) : components{x, y, z, w} {}

  f32& operator[](usize idx)
  {
    return components[idx];
  }
  const f32& operator[](usize idx) const;
};

struct Mat4
{
  union
  {
    f32 data[16];
    Vec4 rows[4];
  };

  Mat4() : rows{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}} {}

  f32& operator[](usize idx);
  const f32& operator[](usize idx) const;

  void rotate(f32 rad, const Vec3& axis);
};

}

#endif
