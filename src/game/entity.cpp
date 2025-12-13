#include "entity.h"

const char* entity_type_to_cstr(EntityType type)
{
  switch (type)
  {
    case ENTITY_TYPE_PLAYER:
      return "PLAYER";
    case ENTITY_TYPE_STATIC_COLLISION:
      return "STATIC_COLLISION";
    case ENTITY_TYPE_INTERACTABLE:
      return "INTERACTABLE";
  }
}

EntityType string_to_entity_type(const String& str, Error& out_error)
{
  if (str == "PLAYER")
  {
    return ENTITY_TYPE_PLAYER;
  }
  else if (str == "STATIC_COLLISION")
  {
    return ENTITY_TYPE_STATIC_COLLISION;
  }
  else if (str == "INTERACTABLE")
  {
    return ENTITY_TYPE_INTERACTABLE;
  }

  out_error = GLOBAL_ERROR_INVALID_DATA;
  return (EntityType) 0;
}

const char* interactable_type_to_cstr(InteractableType type)
{
  switch (type)
  {
    case INTERACTABLE_TYPE_LIGHT_BULB:
      return "LIGHT_BULB";
  }
}

InteractableType string_to_interactable_type(const String& str, Error& out_error)
{
  if (str == "LIGHT_BULB")
  {
    return INTERACTABLE_TYPE_LIGHT_BULB;
  }

  out_error = GLOBAL_ERROR_INVALID_DATA;
  return (InteractableType) 0;
}

BoundingBox bounding_box_from_model(ModelHandle handle)
{
  Vec3 max_corner = {F32_MIN, 0, F32_MIN};
  Vec3 min_corner = {F32_MAX, 0, F32_MAX};
  auto& model = assets_get_model(handle);
  for (usize mesh_idx = 0; mesh_idx < model.meshes.size; ++mesh_idx)
  {
    auto& mesh = assets_get_mesh(model.meshes[mesh_idx]);
    for (usize vertex_idx = 0; vertex_idx < mesh.vertices.size; ++vertex_idx)
    {
      auto& vertex = mesh.vertices[vertex_idx];
      max_corner.x = max(max_corner.x, vertex.position.x);
      min_corner.x = min(min_corner.x, vertex.position.x);
      max_corner.z = max(max_corner.z, vertex.position.z);
      min_corner.z = min(min_corner.z, vertex.position.z);
    }
  }
  return {max_corner.x - min_corner.x, max_corner.z - min_corner.z};
}

bool entities_collide(const Entity& ea, const Entity& eb)
{
  auto& ax = ea.position.x;
  auto& az = ea.position.z;
  auto& bx = eb.position.x;
  auto& bz = eb.position.z;

  return ax - (ea.bounding_box.width / 2.0f) < bx + (eb.bounding_box.width / 2.0f) &&
         ax + (ea.bounding_box.width / 2.0f) > bx - (eb.bounding_box.width / 2.0f) &&
         az - (ea.bounding_box.depth / 2.0f) < bz + (eb.bounding_box.depth / 2.0f) &&
         az + (ea.bounding_box.depth / 2.0f) > bz - (eb.bounding_box.depth / 2.0f);
}

DrawCall draw_call_entity(const Entity& entity, const Camera& camera)
{
  if (!entity.has_model)
  {
    return {};
  }
  DrawCall out = {};
  out.primitive = PRIMITIVE_TRIANGLES;
  out.model_handle = entity.model;
  out.model = mat4_make();
  mat4_translate(out.model, entity.position);
  out.view = camera_look_at(camera);
  out.projection = camera_projection(camera);
  out.emissive = entity.light_bulb_emissive;
  return out;
}

DrawCall draw_call_entity_bounding_box(const Entity& entity, const Camera& camera)
{
  DrawCall out = {};
  out.primitive = PRIMITIVE_TRIANGLES;
  out.wireframe = true;
  out.model_handle = STATIC_MODEL_BOUNDING_BOX;
  out.model = mat4_make();
  mat4_scale(out.model, {entity.bounding_box.width, 1.0f, entity.bounding_box.depth});
  mat4_translate(out.model, entity.position);
  out.view = camera_look_at(camera);
  out.projection = camera_projection(camera);
  return out;
}

// TODO(szulf): this ring is not right! it off by a little bit
DrawCall draw_call_entity_interactable_radius(const Entity& entity, const Camera& camera)
{
  DrawCall out = {};
  out.primitive = PRIMITIVE_LINE_STRIP;
  out.model_handle = STATIC_MODEL_RING;
  out.model = mat4_make();
  mat4_scale(
    out.model,
    {interactable_info[entity.interactable_type].radius2,
     1.0f,
     interactable_info[entity.interactable_type].radius2}
  );
  mat4_translate(out.model, entity.position);
  out.view = camera_look_at(camera);
  out.projection = camera_projection(camera);
  return out;
}
