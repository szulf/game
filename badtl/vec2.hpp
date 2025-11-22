#ifndef BADTL_VEC2_HPP
#define BADTL_VEC2_HPP

#include "types.hpp"

namespace btl {

struct Vec2 {
  f32 x;
  f32 y;

  bool operator==(const Vec2& other) const;
};

void write_formatted_type(usize& buf_idx, char* buffer, usize n, const Vec2& first);

}

#endif
