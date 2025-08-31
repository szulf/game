#include "vec3.h"

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


Vec3& Vec3::operator+=(const Vec3& other)
{
  x += other.x;
  y += other.y;
  z += other.z;
  return *this;
}

Vec3& Vec3::operator-=(const Vec3& other)
{
  x -= other.x;
  y -= other.y;
  z -= other.z;
  return *this;
}

Vec3& Vec3::operator*=(const f32& other)
{
  x *= other;
  y *= other;
  z *= other;
  return *this;
}

Vec3& Vec3::operator/=(const f32& other)
{
  ASSERT(other != 0.0f, "denominator cannot be 0");
  x /= other;
  y /= other;
  z /= other;
  return *this;
}

Vec3 Vec3::operator+(const Vec3& other)
{
  return Vec3(*this) += other;
}

Vec3 Vec3::operator-(const Vec3& other)
{
  return Vec3(*this) -= other;
}

Vec3 Vec3::operator*(const f32& other)
{
  return Vec3(*this) *= other;
}

Vec3 Vec3::operator/(const f32& other)
{
  return Vec3(*this) /= other;
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
