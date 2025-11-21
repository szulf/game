#include "vec3.hpp"

#include "math.hpp"

namespace btl {

f32 Vec3::length() const {
  return btl::sqrt(x * x + y * y + z * z);
}

Vec3 Vec3::normalize() const {
  Vec3 out = {};
  auto len = length();
  out.x = x / len;
  out.y = y / len;
  out.z = z / len;
  return out;
}

bool Vec3::operator==(const Vec3& other) const {
  return f32_eql(x, other.x) && f32_eql(y, other.y) && f32_eql(z, other.z);
}

Vec3 Vec3::operator-() const {
  return Vec3{-x, -y, -z};
}

Vec3 cross(const Vec3& va, const Vec3& vb) {
  return {
    (va.y * vb.z) - (va.z * vb.y),
    (va.z * vb.x) - (va.x * vb.z),
    (va.x * vb.y) - (va.y * vb.x),
  };
}

}
