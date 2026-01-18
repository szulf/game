#ifndef ENTITY_H
#define ENTITY_H

#define PLAYER_MOVEMENT_SPEED 8.0f
#define PLAYER_ROTATE_SPEED (3 * F32_PI)
#define PLAYER_MASS 80.0f

#define LIGHT_BULB_ON_TINT vec3{1.0f, 1.0f, 1.0f}
#define LIGHT_BULB_OFF_TINT vec3{0.1f, 0.1f, 0.1f}

struct BoundingBox
{
  f32 width;
  f32 depth;

  static BoundingBox from_model(assets::ModelHandle model);
};

struct Entity
{
  vec3 pos;

  bool controlled_by_player;
  f32 rotation;
  f32 target_rotation;
  vec3 velocity;

  bool collidable;
  BoundingBox bounding_box;

  bool has_model;
  assets::ModelHandle model;

  bool interactable;
  f32 interactable_radius;

  bool emits_light;
  f32 light_height_offset;
  vec3 light_color;

  vec3 tint = {1.0f, 1.0f, 1.0f};

  // NOTE: read/write
  String name;
  String model_path;
  bool is_bounding_box_from_model;

  static Entity from_file(const char* path, Allocator& allocator, Error& out_error);
};

bool entities_collide(const Entity& ea, const Entity& eb);

Array<renderer::Item> renderer_item_entity(const Entity& entity, Allocator& allocator);
renderer::Item renderer_item_entity_bounding_box(const Entity& entity);
renderer::Item renderer_item_entity_interactable_radius(const Entity& entity);
renderer::Item renderer_item_player_rotation(const Entity& entity);

#define MAX_ENTITIES 1000
struct Scene
{
  vec3 ambient_color;

  Entity entities[MAX_ENTITIES];
  usize entities_count;

  static Scene from_file(const char* path, Allocator& allocator, Error& out_error);
};

#endif
