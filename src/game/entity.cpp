#include "entity.h"

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

template <>
Entity Serializer::read<Entity>(
  const String& str,
  Error& out_error,
  Allocator* allocator,
  SourceType source_type
)
{
  ASSERT(allocator != nullptr, "cannot pass a nullptr to Serializer::read<Entity>()");

  Error error = SUCCESS;
  Entity out = {};

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  String source = take_source(str, source_type, scratch_arena.allocator, out_error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, out);

  auto lines = source.split('\n', scratch_arena.allocator);
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

    // find-error off
    if (key == "pos")
    {
      out.pos = read<vec3>(value, error);
    }
    else if (key == "controlled_by_player")
    {
      out.controlled_by_player = read<bool>(value, error);
    }
    else if (key == "rotation")
    {
      out.rotation = read<f32>(value, error);
    }
    else if (key == "target_rotation")
    {
      out.target_rotation = read<f32>(value, error);
    }
    else if (key == "velocity")
    {
      out.velocity = read<vec3>(value, error);
    }
    else if (key == "collidable")
    {
      out.collidable = read<bool>(value, error);
    }
    else if (key == "bounding_box")
    {
      if (value == "*")
      {
        ASSERT(out.has_model, "gent decoding error. Cannot calculate bounding box without model.");
        out.bounding_box = BoundingBox::from_model(out.model);
        out.is_bounding_box_from_model = true;
      }
      else
      {
        auto bb = read<vec2>(value, error);
        out.bounding_box.width = bb.x;
        out.bounding_box.depth = bb.y;
      }
    }
    else if (key == "has_model")
    {
      out.has_model = read<bool>(value, error);
    }
    else if (key == "model")
    {
      out.model_path = value.copy(*allocator);
      out.model = assets::Model::from_file(
        out.model_path.to_cstr(scratch_arena.allocator),
        *allocator,
        error
      );
    }
    else if (key == "interactable")
    {
      out.interactable = read<bool>(value, error);
    }
    else if (key == "interactable_radius")
    {
      out.interactable_radius = read<f32>(value, error);
    }
    else if (key == "emits_light")
    {
      out.emits_light = read<bool>(value, error);
    }
    else if (key == "light_height_offset")
    {
      out.light_height_offset = read<f32>(value, error);
    }
    else if (key == "light_color")
    {
      out.light_color = read<vec3>(value, error);
    }
    else if (key == "tint")
    {
      out.tint = read<vec3>(value, error);
    }
    else if (key == "name")
    {
      out.name = value.copy(*allocator);
    }
    ERROR_ASSERT(error == SUCCESS, out_error, error, out);
    // find-error on
  }
  return out;
}

template <>
Scene Serializer::read<Scene>(
  const String& str,
  Error& out_error,
  Allocator* allocator,
  SourceType source_type
)
{
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  String source = take_source(str, source_type, scratch_arena.allocator, out_error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});

  auto lines = source.split('\n', scratch_arena.allocator);
  Scene scene = {};
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
      ambient_color = read<vec3>(value, error);
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
      entity = read<Entity>(entity_path, error, allocator, SourceType::FILE);
      ERROR_ASSERT(error == SUCCESS, out_error, error, scene);
      entity_cache.set(key, entity);
    }

    auto pos = read<vec3>(value, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, scene);

    entity.pos = pos;

    scene.entities[scene.entities_count++] = entity;
  }

  for (usize entity_idx = 0; entity_idx < scene.entities_count; ++entity_idx)
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
