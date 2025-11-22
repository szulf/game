#include "vec2.hpp"

#include <stdio.h>

#include "math.hpp"

namespace btl {

bool Vec2::operator==(const Vec2& other) const {
  return f32_eql(x, other.x) && f32_eql(y, other.y);
}

void write_formatted_type(usize& buf_idx, char* buffer, usize n, const Vec2& first) {
  buf_idx += static_cast<usize>(
    snprintf(buffer + buf_idx, n - buf_idx, "{%.2f %.2f}", static_cast<double>(first.x), static_cast<double>(first.y))
  );
}

}
