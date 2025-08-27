#ifndef VEC4_H
#define VEC4_H

namespace math
{

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

  void length();
  void normalize();

  f32& operator[](usize idx);
  const f32& operator[](usize idx) const;
};

}

#endif
