#ifndef BADTL_MAT4_HPP
#define BADTL_MAT4_HPP

#include "array.hpp"
#include "types.hpp"
#include "vec3.hpp"

namespace btl {

struct Mat4 {
  static Mat4 make();
  static Mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far);

  void translate(const Vec3& vec);
  void rotate(f32 rad, const Vec3& axis);

  Array<f32, 16> data;
};

}

#endif
