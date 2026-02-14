#ifndef CAMERA_H
#define CAMERA_H

#include "base/base.h"
#include "base/math.h"

enum class CameraType
{
  PERSPECTIVE,
  ORTHOGRAPHIC,
};

struct CameraDescription
{
  CameraType type{};
  vec3 pos{};
  f32 yaw{};
  f32 pitch{};
  bool using_vertical_fov{};
  f32 fov{};
  f32 near_plane{};
  f32 far_plane{};
  uvec2 viewport{};
};

class Camera
{
public:
  static constexpr vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};
  static constexpr f32 SPEED = 4.0f;
  static constexpr f32 SENSITIVITY = 0.8f;

public:
  Camera() {}
  Camera(const CameraDescription& desc)
    : m_type{desc.type}, m_pos{desc.pos}, m_prev_pos{desc.pos}, m_rendered_pos{desc.pos},
      m_yaw{desc.yaw}, m_pitch{desc.pitch}, m_using_vertical_fov{desc.using_vertical_fov},
      m_fov{desc.fov}, m_near_plane{desc.near_plane}, m_far_plane{desc.far_plane},
      m_viewport{desc.viewport}
  {
    update_vectors();
  }

  inline constexpr void update_viewport(const uvec2& viewport) noexcept
  {
    m_viewport = viewport;
  }

  void update(f32 alpha);
  void look_around(const vec2& offset);
  void move(const vec3& acceleration, const vec3& direction, f32 dt);

  mat4 look_at() const;
  mat4 projection() const;

  [[nodiscard]] inline constexpr uvec2 viewport() const noexcept
  {
    return m_viewport;
  }

  [[nodiscard]] inline constexpr vec3 pos() const noexcept
  {
    return m_pos;
  }

  [[nodiscard]] inline constexpr f32 far_plane() const noexcept
  {
    return m_far_plane;
  }

  [[nodiscard]] inline constexpr vec3 right() const noexcept
  {
    return m_right;
  }

private:
  void update_vectors();

private:
  CameraType m_type{};

  vec3 m_pos{};
  vec3 m_prev_pos{};
  vec3 m_rendered_pos{};

  f32 m_yaw{};
  f32 m_pitch{};

  bool m_using_vertical_fov{};
  f32 m_fov{};
  f32 m_near_plane{};
  f32 m_far_plane{};

  uvec2 m_viewport{};

  vec3 m_front{};
  vec3 m_up{};
  vec3 m_right{};
};

#endif
