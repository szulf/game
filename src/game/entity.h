#ifndef ENTITY_H
#define ENTITY_H

#include "base/base.h"

#include "assets.h"

#define PLAYER_MOVEMENT_SPEED 8.0f
#define PLAYER_ROTATE_SPEED (3 * F32_PI)
#define PLAYER_MASS 80.0f

#define LIGHT_BULB_ON_TINT vec3{1.0f, 1.0f, 1.0f}
#define LIGHT_BULB_OFF_TINT vec3{0.1f, 0.1f, 0.1f}

struct BoundingBox
{
  f32 width;
  f32 depth;

  static BoundingBox from_mesh(MeshHandle mesh, const Assets& assets);
};

struct Entity
{
  vec3 pos;
  vec3 prev_pos;
  vec3 rendered_pos;

  bool controlled_by_player;
  f32 rotation;
  f32 prev_rotation;
  f32 rendered_rotation;
  f32 target_rotation;
  vec3 velocity;

  bool collidable;
  BoundingBox bounding_box;

  bool renderable;
  MeshHandle mesh;

  bool interactable;
  f32 interactable_radius;

  bool emits_light;
  f32 light_height_offset;
  vec3 light_color;

  vec3 tint = {1.0f, 1.0f, 1.0f};

  // NOTE: read/write
  String name;
  String mesh_path;
  bool is_bounding_box_from_model;
};

bool entities_collide(const Entity& ea, const Entity& eb);

#define MAX_ENTITIES 1000
struct Scene
{
  vec3 ambient_color;

  Entity entities[MAX_ENTITIES];
  usize entities_count;
};

Entity load_gent(const char* path, Assets& assets, Allocator& allocator, Error& out_error);
Scene load_gscn(const char* path, Assets& assets, Allocator& allocator, Error& out_error);

#endif
