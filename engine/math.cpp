#include "math.hpp"

#include <cmath>

namespace math {

[[nodiscard]] auto cross(const vec3& va, const vec3& vb) noexcept -> vec3 {
  vec3 out;
  out.x = (va.y * vb.z) - (va.z * vb.y);
  out.y = (va.z * vb.x) - (va.x * vb.z);
  out.z = (va.x * vb.y) - (va.y * vb.x);
  return out;
}

auto mat4::rotate(float rad, const vec3& axis) noexcept -> void {
  float s = std::sin(rad);
  float c = std::cos(rad);
  float t = 1.0f - c;
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
