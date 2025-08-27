#include "math.h"

namespace math
{

template <typename T>
T min(const T& a, const T& b)
{
  if (a < b)
  {
    return b;
  }

  return a;
}

template <typename T>
T max(const T& a, const T& b)
{
  if (a >= b)
  {
    return a;
  }

  return b;
}

bool32 is_power_of_two(usize val)
{
  return (val & (val - 1)) == 0;
}

f32 radians(f32 deg)
{
  return deg * 0.01745329251994329576923690768489f;
}

}
