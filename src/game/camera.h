#ifndef CAMERA_H
#define CAMERA_H

static const Vec3 CAMERA_WORLD_UP = {0.0f, 1.0f, 0.0f};
#define CAMERA_SPEED 4.0f
#define CAMERA_SENSITIVITY 1.5f

struct Camera
{
  Vec3 pos;
  Vec3 front;
  Vec3 up;
  Vec3 right;

  f32 yaw;
  f32 pitch;

  f32 fov;
  f32 near_plane;
  f32 far_plane;

  u32 viewport_width;
  u32 viewport_height;
};

void camera_update_vectors(Camera& camera);

Mat4 camera_look_at(const Camera& camera);
Mat4 camera_projection(const Camera& camera);

#endif
