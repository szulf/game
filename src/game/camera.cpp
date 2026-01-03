#include "camera.h"

void camera_update_vectors(Camera& camera)
{
  camera.front.x = cos(radians(camera.yaw)) * cos(radians(camera.pitch));
  camera.front.y = sin(radians(camera.pitch));
  camera.front.z = sin(radians(camera.yaw)) * cos(radians(camera.pitch));
  camera.front = normalize(camera.front);

  camera.right = normalize(cross(camera.front, CAMERA_WORLD_UP));
  camera.up = normalize(cross(camera.right, camera.front));
}

mat4 camera_look_at(const Camera& camera)
{
  return mat4_look_at(camera.pos, camera.pos + camera.front, camera.up);
}

mat4 camera_projection(const Camera& camera)
{
  switch (camera.type)
  {
    case CAMERA_TYPE_PERSPECTIVE:
    {
      return mat4_perspective(
        camera.fov,
        (f32) camera.viewport_width / (f32) camera.viewport_height,
        camera.near_plane,
        camera.far_plane,
        camera.using_vertical_fov
      );
    }
    break;
    case CAMERA_TYPE_ORTHOGRAPHIC:
    {
      static const f32 world_height = 10.0f;
      static const f32 half_height = world_height * 0.5f;
      f32 half_width = half_height * ((f32) camera.viewport_width / (f32) camera.viewport_height);

      return mat4_orthographic(
        half_width,
        -half_width,
        half_height,
        -half_height,
        camera.near_plane,
        camera.far_plane
      );
    }
    break;
  }
}
