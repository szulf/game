#ifndef VEC3_H
#define VEC3_H

namespace math
{

struct Vec3
{
  union
  {
    struct
    {
      f32 x;
      f32 y;
      f32 z;
    };
    struct
    {
      f32 r;
      f32 g;
      f32 b;
    };
    f32 data[3];
  };

  Vec3(f32 x, f32 y, f32 z) : data{x, y, z} {}

  f32 length() const;
  void normalize();

  f32& operator[](usize idx);
  const f32& operator[](usize idx) const;
};

}

#endif
