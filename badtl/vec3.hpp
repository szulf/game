#ifndef BADTL_VEC3_HPP
#define BADTL_VEC3_HPP

#include "types.hpp"

namespace btl {

struct Vec3 {
  f32 x;
  f32 y;
  f32 z;

  f32 length() const;
  Vec3 normalize() const;
  bool operator==(const Vec3& other) const;
  Vec3 operator-() const;
};

Vec3 cross(const Vec3& va, const Vec3& vb);

}

#endif
