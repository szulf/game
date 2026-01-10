#ifndef ENTITY_H
#define ENTITY_H

enum class EntityType
{
  PLAYER,
  STATIC_COLLISION,
  INTERACTABLE,

  COUNT,
};

const char* entity_type_to_cstr(EntityType type);
EntityType string_to_entity_type(const String& str, Error& out_error);

#define PLAYER_MOVEMENT_SPEED 8.0f
#define PLAYER_ROTATE_SPEED (3 * F32_PI)
#define PLAYER_MASS 80.0f

enum class InteractableType
{
  LIGHT_BULB,
};

const char* interactable_type_to_cstr(InteractableType type);
InteractableType string_to_interactable_type(const String& str, Error& out_error);

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
  EntityType type;

  f32 rotation;
  f32 target_rotation;
  vec3 pos;
  vec3 velocity;

  BoundingBox bounding_box;

  bool has_model;
  assets::ModelHandle model;

  vec3 tint = {1.0f, 1.0f, 1.0f};

  InteractableType interactable_type;
  f32 interactable_radius;

  // NOTE(szulf): lights (?)
  f32 light_height_offset;

  // NOTE(szulf): light_bulb
  bool light_bulb_on;
  vec3 light_bulb_color;

  // NOTE(szulf): read/write
  String name;
  String model_path;
  bool is_bounding_box_from_model;
};

bool entities_collide(const Entity& ea, const Entity& eb);

Array<renderer::Item> renderer_item_entity(const Entity& entity, Allocator& allocator);
renderer::Item renderer_item_entity_bounding_box(const Entity& entity);
renderer::Item renderer_item_entity_interactable_radius(const Entity& entity);
renderer::Item renderer_item_player_rotation(const Entity& entity);

#endif
