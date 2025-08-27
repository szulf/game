#include "mat4.h"

namespace math
{

f32& Mat4::operator[](usize idx)
{
  ASSERT(idx >= 0 && idx < 16, "mat4 index out of bounds");
  return data[idx];
}

const f32& Mat4::operator[](usize idx) const
{
  ASSERT(idx >= 0 && idx < 16, "mat4 index out of bounds");
  return data[idx];
}

void Mat4::rotate(f32 rad, const Vec3& axis)
{
  auto s = sin(rad);
  auto c = cos(rad);
  auto t = 1 - c;
  
  auto u = axis;
  u.normalize();

  rows[0] = {(u.x * u.x) * t + c, (u.x * u.y) * t - u.z * s, (u.x * u.z) * t + u.y * s, rows[0][3]};
  rows[1] = {(u.x * u.y) * t + u.z * s, (u.y * u.y) * t + c, (u.y * u.z) * t - u.x * s, rows[1][3]};
  rows[2] = {(u.x * u.z) * t - u.y * s, (u.y * u.z) * t + u.x * s, (u.z * u.z) * t + c, rows[2][3]};
}

}
