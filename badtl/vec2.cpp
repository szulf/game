#include "vec2.hpp"

#include "math.hpp"

namespace btl {

bool Vec2::operator==(const Vec2& other) const {
  return f32_eql(x, other.x) && f32_eql(y, other.y);
}

}
