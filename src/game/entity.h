#ifndef ENTITY_H
#define ENTITY_H

// TODO(szulf): should entity be a module?

// TODO(szulf): should these be here?
#define PLAYER_MOVEMENT_SPEED 8.0f
#define PLAYER_ROTATE_SPEED (3 * F32_PI)

enum EntityType
{
  ENTITY_TYPE_PLAYER,
  ENTITY_TYPE_STATIC_COLLISION,
  ENTITY_TYPE_INTERACTABLE,
};

const char* entity_type_to_cstr(EntityType type);
EntityType string_to_entity_type(const String& str, Error& out_error);

enum InteractableType
{
  INTERACTABLE_TYPE_LIGHT_BULB,
};

const char* interactable_type_to_cstr(InteractableType type);
InteractableType string_to_interactable_type(const String& str, Error& out_error);

// TODO(szulf): should this not be just read from the gent file?
#define LIGHT_BULB_RADIUS2 1.0f
#define LIGHT_BULB_TINT_ON vec3{1.0f, 1.0f, 1.0f}
#define LIGHT_BULB_TINT_OFF vec3{0.1f, 0.1f, 0.1f}

struct BoundingBox
{
  f32 width;
  f32 depth;
};

BoundingBox bounding_box_from_model(assets::ModelHandle model);

struct Entity
{
  EntityType type;

  f32 rotation;
  f32 target_rotation;
  vec3 position;
  vec3 velocity;

  BoundingBox bounding_box;

  bool has_model;
  assets::ModelHandle model;

  vec3 tint = {1.0f, 1.0f, 1.0f};

  InteractableType interactable_type;

  // NOTE(szulf): light_bulb
  bool light_bulb_on;
  vec3 light_bulb_color;
  // TODO(szulf): get this from the gent file?
  float light_bulb_height_offset = -0.25f;
  vec3 light_bulb_on_tint;
  vec3 light_bulb_off_tint;

  // NOTE(szulf): used for read/write
  String name;
  String model_path;
  bool is_bounding_box_from_model;
};

bool entities_collide(const Entity& ea, const Entity& eb);

Array<renderer::Item> renderer_item_entity(const Entity& entity, Allocator& allocator);
Array<renderer::Item> renderer_item_entity_bounding_box(const Entity& entity, Allocator& allocator);
Array<renderer::Item>
renderer_item_entity_interactable_radius(const Entity& entity, Allocator& allocator);
// TODO(szulf): draw a line pointing the rotation of the player

#endif
