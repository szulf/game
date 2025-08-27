#include "vec3.h"

namespace math
{

f32 Vec3::length() const
{
  return sqrt((x * x) + (y * y) + (z * z));
}

void Vec3::normalize()
{
  auto len = length();

  x = x / len;
  y = y / len;
  z = z / len;
}

f32& Vec3::operator[](usize idx)
{
  ASSERT(idx >= 0 && idx < 3, "vec3 index out of bounds");
  return data[idx];
}

const f32& Vec3::operator[](usize idx) const
{
  ASSERT(idx >= 0 && idx < 3, "vec3 index out of bounds");
  return data[idx];
}

}
