#ifndef CAMERA_H
#define CAMERA_H

static const vec3 CAMERA_WORLD_UP = {0.0f, 1.0f, 0.0f};
#define CAMERA_SPEED 4.0f
#define CAMERA_SENSITIVITY 0.8f

enum CameraType
{
  CAMERA_TYPE_PERSPECTIVE,
  CAMERA_TYPE_ORTHOGRAPHIC,
};

struct Camera
{
  CameraType type;

  vec3 pos;
  vec3 front;
  vec3 up;
  vec3 right;

  f32 yaw;
  f32 pitch;

  bool using_vertical_fov;
  f32 fov;
  f32 near_plane;
  f32 far_plane;

  u32 viewport_width;
  u32 viewport_height;
};

void camera_update_vectors(Camera& camera);

mat4 camera_look_at(const Camera& camera);
mat4 camera_projection(const Camera& camera);

#endif
