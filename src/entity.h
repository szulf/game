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

struct Entity
{
  EntityType type;

  // TODO(szulf): quaternions for orientation? or just euler angles?
  Vec3 position;
  f32 scale;

  f32 bounding_box_width;
  f32 bounding_box_depth;

  bool has_model;
  ModelHandle model;

  InteractableType interactable_type;
};

bool collides(const Entity& ea, const Entity& eb);

// TODO(szulf): should these functions be here?
DrawCall draw_call_entity(const Entity& entity, const Camera& camera);
DrawCall draw_call_entity_bounding_box(const Entity& entity, const Camera& camera);

#endif
