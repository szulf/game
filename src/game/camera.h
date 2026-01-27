#ifndef CAMERA_H
#define CAMERA_H

#include "base/base.h"
#include "base/math.h"

#define CAMERA_WORLD_UP vec3{0.0f, 1.0f, 0.0f}
#define CAMERA_SPEED 4.0f
#define CAMERA_SENSITIVITY 0.8f

enum class CameraType
{
  PERSPECTIVE,
  ORTHOGRAPHIC,
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

  mat4 look_at() const;
  mat4 projection() const;

  void update_vectors();
};

#endif
