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

  bool has_model;
  ModelHandle model;

  // TODO(szulf): is this a good idea?
  InteractableType interactable_type;
};

// TODO(szulf): should this function be here?
DrawCall draw_call_make(const Entity& entity, const Camera& camera);

#endif
