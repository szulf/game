#include "mat4.h"

Vec4& Mat4::operator[](usize idx)
{
  ASSERT(idx >= 0 && idx < 3, "mat4 index out of bounds");
  return rows[idx];
}

const Vec4& Mat4::operator[](usize idx) const
{
  ASSERT(idx >= 0 && idx < 3, "mat4 index out of bounds");
  return rows[idx];
}

Vec4 Mat4::cols(usize idx)
{
  ASSERT(idx >= 0 && idx < 4, "mat4 col index out of bounds");
  return {data[idx * 4], data[1 + idx * 4], data[2 + idx * 4], data[3 + idx * 4]};
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

void Mat4::translate(const Vec3& vec)
{
  data[12] = vec.x;
  data[13] = vec.y;
  data[14] = vec.z;
}

Mat4 Mat4::perspective(f32 fov, f32 aspect, f32 near, f32 far)
{
  f32 right = near * tan(fov / 2);
  f32 top = right / aspect;

  Mat4 mat{};
  mat.data[0] = near / right;
  mat.data[5] = near / top;
  mat.data[10] = -(far + near) / (far - near);
  mat.data[11] = -1.0f;
  mat.data[14] = -(2.0f * far * near) / (far - near);
  mat.data[15] = 0;

  return mat;
}
