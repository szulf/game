#include "camera.h"

mat4 Camera::look_at() const
{
  return mat4::look_at(rendered_pos, rendered_pos + front, up);
}

mat4 Camera::projection() const
{
  switch (type)
  {
    case CameraType::PERSPECTIVE:
    {
      return mat4::perspective(
        fov,
        (f32) viewport_width / (f32) viewport_height,
        near_plane,
        far_plane,
        using_vertical_fov
      );
    }
    break;
    case CameraType::ORTHOGRAPHIC:
    {
      static const f32 world_height = 10.0f;
      static const f32 half_height = world_height * 0.5f;
      f32 half_width = half_height * ((f32) viewport_width / (f32) viewport_height);

      return mat4::orthographic(
        half_width,
        -half_width,
        half_height,
        -half_height,
        near_plane,
        far_plane
      );
    }
    break;
  }
}

void Camera::update_vectors()
{
  front.x = cos(radians(yaw)) * cos(radians(pitch));
  front.y = sin(radians(pitch));
  front.z = sin(radians(yaw)) * cos(radians(pitch));
  front = normalize(front);

  right = normalize(cross(front, CAMERA_WORLD_UP));
  up = normalize(cross(right, front));
}
