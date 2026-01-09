#include "entity.h"

const char* entity_type_to_cstr(EntityType type)
{
  switch (type)
  {
    case EntityType::PLAYER:
      return "PLAYER";
    case EntityType::STATIC_COLLISION:
      return "STATIC_COLLISION";
    case EntityType::INTERACTABLE:
      return "INTERACTABLE";
  }
}

EntityType string_to_entity_type(const String& str, Error& out_error)
{
  if (str == "PLAYER")
  {
    return EntityType::PLAYER;
  }
  else if (str == "STATIC_COLLISION")
  {
    return EntityType::STATIC_COLLISION;
  }
  else if (str == "INTERACTABLE")
  {
    return EntityType::INTERACTABLE;
  }

  out_error = GLOBAL_ERROR_INVALID_DATA;
  return (EntityType) 0;
}

const char* interactable_type_to_cstr(InteractableType type)
{
  switch (type)
  {
    case InteractableType::LIGHT_BULB:
      return "LIGHT_BULB";
  }
}

InteractableType string_to_interactable_type(const String& str, Error& out_error)
{
  if (str == "LIGHT_BULB")
  {
    return InteractableType::LIGHT_BULB;
  }

  out_error = GLOBAL_ERROR_INVALID_DATA;
  return (InteractableType) 0;
}

BoundingBox BoundingBox::from_model(assets::ModelHandle handle)
{
  vec3 max_corner = {F32_MIN, 0, F32_MIN};
  vec3 min_corner = {F32_MAX, 0, F32_MAX};
  auto& model = assets::model_get(handle);

  for (usize mesh_idx = 0; mesh_idx < model.parts.size; ++mesh_idx)
  {
    auto& mesh = assets::mesh_get(model.parts[mesh_idx].mesh);
    for (usize vertex_idx = 0; vertex_idx < mesh.vertices.size; ++vertex_idx)
    {
      auto& vertex = mesh.vertices[vertex_idx];
      max_corner.x = max(max_corner.x, vertex.pos.x);
      min_corner.x = min(min_corner.x, vertex.pos.x);
      max_corner.z = max(max_corner.z, vertex.pos.z);
      min_corner.z = min(min_corner.z, vertex.pos.z);
    }
  }
  return {max_corner.x - min_corner.x, max_corner.z - min_corner.z};
}

bool entities_collide(const Entity& ea, const Entity& eb)
{
  auto& ax = ea.pos.x;
  auto& az = ea.pos.z;
  auto& bx = eb.pos.x;
  auto& bz = eb.pos.z;

  return ax - (ea.bounding_box.width / 2.0f) < bx + (eb.bounding_box.width / 2.0f) &&
         ax + (ea.bounding_box.width / 2.0f) > bx - (eb.bounding_box.width / 2.0f) &&
         az - (ea.bounding_box.depth / 2.0f) < bz + (eb.bounding_box.depth / 2.0f) &&
         az + (ea.bounding_box.depth / 2.0f) > bz - (eb.bounding_box.depth / 2.0f);
}

Array<renderer::Item> renderer_item_entity(const Entity& entity, Allocator& allocator)
{
  auto& model = assets::model_get(entity.model);
  auto out = Array<renderer::Item>::make(ArrayType::STATIC, model.parts.size, allocator);
  for (usize part_idx = 0; part_idx < model.parts.size; ++part_idx)
  {
    auto& part = model.parts[part_idx];
    renderer::Item renderer_item = {};
    renderer_item.model = mat4::make();
    renderer_item.tint = entity.tint;
    translate(renderer_item.model, entity.pos);
    rotate(renderer_item.model, entity.rotation, {0.0f, 1.0f, 0.0f});
    renderer_item.mesh = part.mesh;
    renderer_item.material = part.material;
    out.push(renderer_item);
  }
  return out;
}

renderer::Item renderer_item_entity_bounding_box(const Entity& entity)
{
  auto& model = assets::model_get(renderer::STATIC_MODEL_BOUNDING_BOX);
  auto& part = model.parts[0];
  renderer::Item out = {};
  out.model = mat4::make();
  scale(out.model, {entity.bounding_box.width, 1.0f, entity.bounding_box.depth});
  translate(out.model, entity.pos);
  out.tint = {1.0f, 1.0f, 1.0f};
  out.mesh = part.mesh;
  out.material = part.material;
  return out;
}

renderer::Item renderer_item_entity_interactable_radius(const Entity& entity)
{
  auto& model = assets::model_get(renderer::STATIC_MODEL_RING);
  auto& part = model.parts[0];
  renderer::Item out = {};
  out.model = mat4::make();
  auto diameter = 2.0f * sqrt(entity.interactable_radius);
  scale(out.model, {diameter, 1.0f, diameter});
  translate(out.model, entity.pos);
  out.tint = {1.0f, 1.0f, 1.0f};
  out.mesh = part.mesh;
  out.material = part.material;
  return out;
}

renderer::Item renderer_item_player_rotation(const Entity& entity)
{
  auto& model = assets::model_get(renderer::STATIC_MODEL_LINE);
  auto& part = model.parts[0];
  renderer::Item out = {};
  out.model = mat4::make();
  rotate(out.model, entity.rotation, {0.0f, 1.0f, 0.0f});
  scale(out.model, 0.3f);
  vec3 translate_vec = {-sin(entity.rotation) * 0.5f, 0.0f, cos(entity.rotation) * 0.5f};
  translate(out.model, translate_vec + entity.pos);
  out.tint = {1.0f, 1.0f, 1.0f};
  out.mesh = part.mesh;
  out.material = part.material;
  return out;
}
