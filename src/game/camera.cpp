#include "camera.h"

void camera_update_vectors(Camera& camera)
{
  camera.front.x = f32_cos(f32_radians(camera.yaw)) * f32_cos(f32_radians(camera.pitch));
  camera.front.y = f32_sin(f32_radians(camera.pitch));
  camera.front.z = f32_sin(f32_radians(camera.yaw)) * f32_cos(f32_radians(camera.pitch));
  camera.front = vec3_normalize(camera.front);

  camera.right = vec3_normalize(vec3_cross(camera.front, CAMERA_WORLD_UP));
  camera.up = vec3_normalize(vec3_cross(camera.right, camera.front));
}

Mat4 camera_look_at(const Camera& camera)
{
  Vec3 s = vec3_normalize(vec3_cross(camera.front, camera.up));
  Vec3 u = vec3_cross(s, camera.front);

  Mat4 look_at = {};

  look_at.data[0] = s.x;
  look_at.data[4] = s.y;
  look_at.data[8] = s.z;
  look_at.data[12] = -vec3_dot(s, camera.pos);

  look_at.data[1] = u.x;
  look_at.data[5] = u.y;
  look_at.data[9] = u.z;
  look_at.data[13] = -vec3_dot(u, camera.pos);

  look_at.data[2] = -camera.front.x;
  look_at.data[6] = -camera.front.y;
  look_at.data[10] = -camera.front.z;
  look_at.data[14] = vec3_dot(camera.front, camera.pos);

  look_at.data[3] = 0.0f;
  look_at.data[7] = 0.0f;
  look_at.data[11] = 0.0f;
  look_at.data[15] = 1.0f;

  return look_at;
}

Mat4 camera_projection(const Camera& camera)
{
  return mat4_perspective(
    camera.fov,
    (f32) camera.viewport_width / (f32) camera.viewport_height,
    camera.near_plane,
    camera.far_plane
  );
}
