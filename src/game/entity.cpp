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
    case EntityType::COUNT:
      ASSERT(false, "no");
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

  out_error = "Invalid entity type string.";
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

  out_error = "Invalid interactable type string.";
  return (InteractableType) 0;
}

BoundingBox BoundingBox::from_model(assets::ModelHandle handle)
{
  vec3 max_corner = {F32_MIN, 0, F32_MIN};
  vec3 min_corner = {F32_MAX, 0, F32_MAX};
  const auto& model = assets::model_get(handle);

  for (usize mesh_idx = 0; mesh_idx < model.parts.size; ++mesh_idx)
  {
    const auto& mesh = assets::mesh_get(model.parts[mesh_idx].mesh);
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

static vec3 gformat_get_vec3_(const String& value, Error& out_error)
{
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  vec3 out = {};

  auto values = value.split(' ', scratch_arena.allocator);
  ERROR_ASSERT(values.size == 3, out_error, "gent decoding error. Invalid vec3.", out);

  out.x = parse_f32(values[0], error);
  ERROR_ASSERT(error == SUCCESS, out_error, "gent decoding error. Invalid vec3.", out);
  out.y = parse_f32(values[1], error);
  ERROR_ASSERT(error == SUCCESS, out_error, "gent decoding error. Invalid vec3.", out);
  out.z = parse_f32(values[2], error);
  ERROR_ASSERT(error == SUCCESS, out_error, "gent decoding error. Invalid vec3.", out);

  return out;
}

Entity Entity::from_file(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  Entity out = {};

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  out.name = String::make(path).get_filename().copy(allocator);

  auto file = platform::read_file_to_string(path, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, out);
  auto lines = file.split('\n', scratch_arena.allocator);

  for (usize line_idx = 0; line_idx < lines.size; ++line_idx)
  {
    auto& line = lines[line_idx];
    if (line[0] == '#')
    {
      continue;
    }

    auto parts = line.split(':', scratch_arena.allocator);
    ERROR_ASSERT(parts.size == 2, out_error, "gent decoding error. Invalid line.", out);

    auto key = parts[0].trim_whitespace();
    auto value = parts[1].trim_whitespace();

    if (key == "pos")
    {
      out.pos = gformat_get_vec3_(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, out);
    }
    else if (key == "model")
    {
      out.has_model = true;
      out.model_path = value.copy(allocator);
      out.model =
        assets::Model::from_file(out.model_path.to_cstr(scratch_arena.allocator), allocator, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, out);
    }
    else if (key == "type")
    {
      out.type = string_to_entity_type(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, out);
    }
    else if (key == "interactable_type")
    {
      ASSERT(
        out.type == EntityType::INTERACTABLE,
        "cannot set interactable_type on non interactable entity"
      );
      out.interactable_type = string_to_interactable_type(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, out);
    }
    else if (key == "bounding_box")
    {
      if (value == "*")
      {
        ERROR_ASSERT(
          out.has_model,
          out_error,
          "gent decoding error. Cannot calculate bounding box without model.",
          out
        );
        out.bounding_box = BoundingBox::from_model(out.model);
        out.is_bounding_box_from_model = true;
      }
      else
      {
        ASSERT(false, "[TODO] load hardcoded bounding box");
      }
    }
    else if (key == "tint")
    {
      out.tint = gformat_get_vec3_(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, out);
    }
    else if (key == "interactable_radius")
    {
      ASSERT(
        out.type == EntityType::INTERACTABLE,
        "cannot set 'interactable_radius' on non interactable entity"
      );
      out.interactable_radius = parse_f32(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, out);
    }
    else if (key == "light_height_offset")
    {
      ASSERT(
        out.type == EntityType::INTERACTABLE &&
          out.interactable_type == InteractableType::LIGHT_BULB,
        "cannot set 'light_height_offset' on non light entity"
      );
      out.light_height_offset = parse_f32(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, out);
    }
    else if (key == "light_bulb_color")
    {
      ASSERT(
        out.type == EntityType::INTERACTABLE &&
          out.interactable_type == InteractableType::LIGHT_BULB,
        "cannot set light_bulb_color on non light_bulb entity"
      );
      out.light_bulb_color = gformat_get_vec3_(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, out);
    }
    else
    {
      out_error = "gent decoding error. Invalid key.";
      return out;
    }
  }
  return out;
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
  const auto& model = assets::model_get(entity.model);
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
  const auto& model = assets::model_get(renderer::STATIC_MODEL_BOUNDING_BOX);
  const auto& part = model.parts[0];
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
  const auto& model = assets::model_get(renderer::STATIC_MODEL_RING);
  const auto& part = model.parts[0];
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
  const auto& model = assets::model_get(renderer::STATIC_MODEL_LINE);
  const auto& part = model.parts[0];
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

Scene Scene::from_file(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto file = platform::read_file_to_string(path, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  auto lines = file.split('\n', scratch_arena.allocator);

  Scene scene = {};
  usize entities_count = 0;
  auto entity_cache = Map<String, Entity>::make(lines.size * 2, scratch_arena.allocator);
  vec3 ambient_color = {};
  for (usize line_idx = 0; line_idx < lines.size; ++line_idx)
  {
    Entity entity = {};
    auto& line = lines[line_idx];
    if (line[0] == '#')
    {
      continue;
    }

    auto parts = line.split(':', scratch_arena.allocator);
    ERROR_ASSERT(parts.size == 2, out_error, "gscn decoding error. Invalid line.", scene);
    auto key = parts[0].trim_whitespace();
    auto value = parts[1].trim_whitespace();

    if (key == "ambient_color")
    {
      ambient_color = gformat_get_vec3_(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, scene);
      continue;
    }

    if (entity_cache.contains(key))
    {
      entity = *entity_cache[key];
    }
    else
    {
      auto entity_path =
        key.prepend("data/", scratch_arena.allocator).append(".gent", scratch_arena.allocator);
      entity = Entity::from_file(entity_path.to_cstr(scratch_arena.allocator), allocator, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, scene);
      entity_cache.set(key, entity);
    }

    vec3 pos = gformat_get_vec3_(value, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, scene);

    entity.pos = pos;

    scene.entities[entities_count++] = entity;
  }

  for (usize entity_idx = 0; entity_idx < entities_count; ++entity_idx)
  {
    auto& model = assets::model_get(scene.entities[entity_idx].model);
    for (usize part_idx = 0; part_idx < model.parts.size; ++part_idx)
    {
      auto& material = assets::material_get(model.parts[part_idx].material);
      material.ambient_color = ambient_color;
    }
  }

  return scene;
}
