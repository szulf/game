#include "renderer/camera.hpp"

namespace core {

math::mat4 Camera::lookAtMatrix() const noexcept {
  if (type == Type::Perspective) {
    math::mat4 look_at;

    math::vec3 back = -front;

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
  }
  std::unreachable();
}

}
