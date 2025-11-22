#include "engine/renderer/camera.hpp"

#include <stdio.h>

namespace core {

constexpr btl::Vec3 Camera::WORLD_UP;

btl::Mat4 Camera::look_at_matrix() const {
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

void write_formatted_type(btl::usize& buf_idx, char* buffer, btl::usize n, const core::Camera& first) {
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "pos"));
  btl::write_formatted_type(buf_idx, buffer, n, first.pos);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, ", front"));
  btl::write_formatted_type(buf_idx, buffer, n, first.front);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, ", up"));
  btl::write_formatted_type(buf_idx, buffer, n, first.up);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, ", right"));
  btl::write_formatted_type(buf_idx, buffer, n, first.right);

  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, ", yaw{"));
  btl::write_formatted_type(buf_idx, buffer, n, first.yaw);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}, pitch{"));
  btl::write_formatted_type(buf_idx, buffer, n, first.pitch);

  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}, fov{"));
  btl::write_formatted_type(buf_idx, buffer, n, first.fov);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}, near_plane{"));
  btl::write_formatted_type(buf_idx, buffer, n, first.near_plane);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}, far_plane{"));
  btl::write_formatted_type(buf_idx, buffer, n, first.far_plane);

  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}, viewport_width{"));
  btl::write_formatted_type(buf_idx, buffer, n, first.viewport_width);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}, viewport_height{"));
  btl::write_formatted_type(buf_idx, buffer, n, first.viewport_height);

  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}, type{"));
  btl::write_formatted_type(buf_idx, buffer, n, first.type);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}"));
}

}
