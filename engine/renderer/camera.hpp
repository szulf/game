#pragma once

#include "badtl/math.hpp"
#include "badtl/vec3.hpp"
#include "badtl/mat4.hpp"
#include "badtl/types.hpp"

namespace core {

struct Camera {
  enum class Type {
    Perspective,
  };

  static Camera make(Type type, const btl::Vec3& pos, btl::u32 width, btl::u32 height) {
    Camera out = {};
    out.type = type;
    out.pos = pos;

    out.front = {0.0f, 0.0f, -1.0f};
    out.yaw = -90.0f;
    out.fov = 45.0f;
    out.near_plane = 0.1f;
    out.far_plane = 1000.0f;

    out.viewport_width = width;
    out.viewport_height = height;

    out.update_camera_vectors();
    return out;
  }

  btl::Mat4 projection_matrix() const {
    ASSERT(type == Type::Perspective, "illegal camera type");

    return btl::Mat4::perspective(
      fov,
      static_cast<btl::f32>(viewport_width) / static_cast<btl::f32>(viewport_height),
      near_plane,
      far_plane
    );
  }

  btl::Mat4 look_at_matrix() const;

  void update_camera_vectors() {
    front.x = btl::cos(btl::radians(yaw)) * btl::cos(btl::radians(pitch));
    front.y = btl::sin(btl::radians(pitch));
    front.z = btl::sin(btl::radians(yaw)) * btl::cos(btl::radians(pitch));
    front = front.normalize();
    right = btl::cross(front, WORLD_UP).normalize();
    up = btl::cross(right, front).normalize();
  }

  btl::Vec3 pos;
  btl::Vec3 front;
  btl::Vec3 up;
  btl::Vec3 right;

  btl::f32 yaw;
  btl::f32 pitch;

  btl::f32 fov;
  btl::f32 near_plane;
  btl::f32 far_plane;

  btl::u32 viewport_width;
  btl::u32 viewport_height;

  Type type;
  constexpr static btl::Vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};
};

void write_formatted_type(btl::usize& buf_idx, char* buffer, btl::usize n, const core::Camera& first);

}
