#ifndef MAT4_H
#define MAT4_H

struct Mat4
{
  union
  {
    f32 data[16];
    Vec4 rows[4];
  };

  Mat4() : rows{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}} {}

  static Mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far);

  Vec4 cols(usize idx);
  void rotate(f32 rad, const Vec3& axis);
  void translate(const Vec3& vec);

  Vec4& operator[](usize idx);
  const Vec4& operator[](usize idx) const;

};

#endif
