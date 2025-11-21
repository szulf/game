#include "mat4.hpp"

#include "math.hpp"

namespace btl {

Mat4 Mat4::make() {
  Mat4 out = {};
  out.data[0] = 1.0f;
  out.data[5] = 1.0f;
  out.data[10] = 1.0f;
  out.data[15] = 1.0f;
  return out;
}

Mat4 Mat4::perspective(f32 fov, f32 aspect, f32 near, f32 far) {
  Mat4 out = {};
  f32 right = near * btl::tan(fov / 2.0f);
  f32 top = right / aspect;
  out.data[0] = near / right;
  out.data[5] = near / top;
  out.data[10] = -(far + near) / (far - near);
  out.data[11] = -1.0f;
  out.data[14] = -(2.0f * far * near) / (far - near);
  out.data[15] = 0.0f;
  return out;
}

void Mat4::translate(const Vec3& vec) {
  data[12] = vec.x;
  data[13] = vec.y;
  data[14] = vec.z;
}

void Mat4::rotate(f32 rad, const Vec3& axis) {
  f32 s = btl::sin(rad);
  f32 c = btl::cos(rad);
  f32 t = 1.0f - c;
  auto u = axis.normalize();

  data[0] = (u.x * u.x) * t + c;
  data[1] = (u.x * u.y) * t - u.z * s;
  data[2] = (u.x * u.z) * t + u.y * s;

  data[4] = (u.x * u.y) * t + u.z * s;
  data[5] = (u.y * u.y) * t + c;
  data[6] = (u.y * u.z) * t - u.x * s;

  data[8] = (u.x * u.z) * t - u.y * s;
  data[9] = (u.y * u.z) * t + u.x * s;
  data[10] = (u.z * u.z) * t + c;
}

}
