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
  vec3 s = normalize(cross(camera.front, camera.up));
  vec3 u = cross(s, camera.front);

  mat4 look_at = {};

  look_at.data[0] = s.x;
  look_at.data[4] = s.y;
  look_at.data[8] = s.z;
  look_at.data[12] = -dot(s, camera.pos);

  look_at.data[1] = u.x;
  look_at.data[5] = u.y;
  look_at.data[9] = u.z;
  look_at.data[13] = -dot(u, camera.pos);

  look_at.data[2] = -camera.front.x;
  look_at.data[6] = -camera.front.y;
  look_at.data[10] = -camera.front.z;
  look_at.data[14] = dot(camera.front, camera.pos);

  look_at.data[3] = 0.0f;
  look_at.data[7] = 0.0f;
  look_at.data[11] = 0.0f;
  look_at.data[15] = 1.0f;

  return look_at;
}

mat4 camera_projection(const Camera& camera)
{
  switch (camera.type)
  {
    case CAMERA_TYPE_PERSPECTIVE:
    {
      return mat4_vertical_perspective(
        camera.vertical_fov,
        (f32) camera.viewport_width / (f32) camera.viewport_height,
        camera.near_plane,
        camera.far_plane
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
