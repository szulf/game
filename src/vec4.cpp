#include "vec4.h"

f32 Vec4::length()
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

Vec4& Vec4::operator+=(const Vec4& other)
{
  x += other.x;
  y += other.y;
  z += other.z;
  w += other.w;
  return *this;
}

Vec4& Vec4::operator-=(const Vec4& other)
{
  x -= other.x;
  y -= other.y;
  z -= other.z;
  w -= other.w;
  return *this;
}

Vec4& Vec4::operator*=(const f32& other)
{
  x *= other;
  y *= other;
  z *= other;
  w *= other;
  return *this;
}

Vec4& Vec4::operator/=(const f32& other)
{
  ASSERT(other != 0.0f, "denominator cannot be 0");
  x /= other;
  y /= other;
  z /= other;
  w /= other;
  return *this;
}

Vec4 Vec4::operator+(const Vec4& other)
{
  return Vec4(*this) += other;
}

Vec4 Vec4::operator-(const Vec4& other)
{
  return Vec4(*this) -= other;
}

Vec4 Vec4::operator*(const f32& other)
{
  return Vec4(*this) *= other;
}

Vec4 Vec4::operator/(const f32& other)
{
  return Vec4(*this) /= other;
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
