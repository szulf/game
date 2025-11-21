#include "engine/renderer/camera.hpp"

namespace core {

constexpr btl::Vec3 Camera::WORLD_UP;

btl::Mat4 Camera::lookAtMatrix() const {
  switch (type) {
    case Type::Perspective: {
      btl::Mat4 look_at = {};

      btl::Vec3 back = -front;

      look_at.data[0] = right.x;
      look_at.data[1] = right.y;
      look_at.data[2] = right.z;

      look_at.data[4] = up.x;
      look_at.data[5] = up.y;
      look_at.data[6] = up.z;

      look_at.data[8] = back.x;
      look_at.data[9] = back.y;
      look_at.data[10] = back.z;

      look_at.data[12] = pos.x;
      look_at.data[13] = pos.y;
      look_at.data[14] = pos.z;

      look_at.data[15] = 1.0f;

      return look_at;
    } break;
  }

  ASSERT(false, "unexpected camera type");
  return {};
}

}
