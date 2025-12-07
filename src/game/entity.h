#ifndef ENTITY_H
#define ENTITY_H

#define PLAYER_SPEED 3.0f

enum EntityType
{
  ENTITY_TYPE_PLAYER,
  ENTITY_TYPE_STATIC_COLLISION,
  ENTITY_TYPE_INTERACTABLE,
};

enum InteractableType
{
  INTERACTABLE_TYPE_LIGHT_BULB,
};

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
  f32 scale;

  BoundingBox bounding_box;

  bool has_model;
  ModelHandle model;

  InteractableType interactable_type;
};

bool collides(const Entity& ea, const Entity& eb);

DrawCall draw_call_entity(const Entity& entity, const Camera& camera);
DrawCall draw_call_entity_bounding_box(const Entity& entity, const Camera& camera);
DrawCall draw_call_entity_interactable_radius(const Entity& entity, const Camera& camera);

#endif
