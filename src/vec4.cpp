#include "vec4.h"
  
namespace math
{

void Vec4::length()
{
  return sqrt(x * x + y * y + z * z + w * w);
}

void Vec4::normalize()
{
  auto len = length();

  x = x / len;
  y = y / len;
  z = z / len;
  w = w / len;
}

f32& Vec4::operator[](usize idx)
{
  ASSERT(idx >= 0 && idx < 4, "vec4 index out of bounds");
  return data[idx];
}

const f32& Vec4::operator[](usize idx) const
{
  ASSERT(idx >= 0 && idx < 4, "vec4 index out of bounds");
  return data[idx];
}

}
