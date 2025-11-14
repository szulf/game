#pragma once

#include <utility>

#include "math.hpp"
#include "assert.hpp"

namespace core {

struct Camera final {
  enum class Type {
    Perspective,
    Orthographic,
  };

  Camera(Type t) noexcept : type{t} {
    ASSERT(type == Type::Orthographic, "this constructo needs to construct an orthographic camera");
  }
  Camera(Type t, math::vec3 p) noexcept : pos{p}, type{t} {
    ASSERT(type == Type::Perspective, "this constructor needs to construct a perspective camera");
    updateCameraVectors();
  }

  math::mat4 projectionMatrix() const noexcept {
    if (type == Type::Perspective) {
      return math::mat4{
        fov,
        static_cast<float>(viewport_width) / static_cast<float>(viewport_height),
        near_plane,
        far_plane
      };
    }
    std::unreachable();
  }

  math::mat4 lookAtMatrix() const noexcept;

  void updateCameraVectors() {
    front.x = std::cos(math::radians(yaw)) * std::cos(math::radians(pitch));
    front.y = std::sin(math::radians(pitch));
    front.z = std::sin(math::radians(yaw)) * std::cos(math::radians(pitch));
    front = front.normalize();
    right = math::cross(front, world_up).normalize();
    up = math::cross(right, front).normalize();
  }

  math::vec3 pos{};
  math::vec3 front{0.0f, 0.0f, -1.0f};
  math::vec3 up{};
  math::vec3 right{};
  math::vec3 world_up{0.0f, 1.0f, 0.0f};

  float yaw{-90.0f};
  float pitch{};

  float fov{45.0f};
  float near_plane{0.1f};
  float far_plane{1000.0f};

  std::uint32_t viewport_width{};
  std::uint32_t viewport_height{};

  Type type{};
};

}
