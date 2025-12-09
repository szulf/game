#ifndef ENTITY_H
#define ENTITY_H

// TODO(szulf): should entity be a module?

#define PLAYER_SPEED 3.0f

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

struct InteractableInfo
{
  f32 radius2;
};

static const InteractableInfo interactable_info[] = {
  {1.0f},
};

struct BoundingBox
{
  f32 width;
  f32 depth;
};

BoundingBox bounding_box_from_model(ModelHandle model);

struct Entity
{
  EntityType type;

  // TODO(szulf): quaternions for orientation? or just euler angles?
  Vec3 position;

  BoundingBox bounding_box;

  bool has_model;
  ModelHandle model;

  InteractableType interactable_type;

  // NOTE(szulf): used for read/write
  String name;
  String model_path;
  bool is_bounding_box_from_model;
};

bool collides(const Entity& ea, const Entity& eb);

DrawCall draw_call_entity(const Entity& entity, const Camera& camera);
DrawCall draw_call_entity_bounding_box(const Entity& entity, const Camera& camera);
DrawCall draw_call_entity_interactable_radius(const Entity& entity, const Camera& camera);

enum EntityReadError
{
  ENTITY_READ_ERROR_INVALID_POSITION = GLOBAL_ERROR_COUNT,
  ENTITY_READ_ERROR_INVALID_MODEL,
  ENTITY_READ_ERROR_INVALID_TYPE,
  ENTITY_READ_ERROR_INVALID_INTERACTABLE_TYPE,
  ENTITY_READ_ERROR_DYNAMIC_BOUNDING_BOX_NO_MODEL,
  ENTITY_READ_ERROR_INVALID_PATH,
};

#endif
