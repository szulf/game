#include "entity.h"

#include "platform/platform.h"

BoundingBox BoundingBox::from_mesh(MeshHandle handle, const Assets& assets)
{
  vec3 max_corner = {F32_MIN, 0, F32_MIN};
  vec3 min_corner = {F32_MAX, 0, F32_MAX};
  const auto& mesh = assets.meshes.get(handle);

  for (usize vertex_idx = 0; vertex_idx < mesh.vertices.size; ++vertex_idx)
  {
    auto& vertex = mesh.vertices[vertex_idx];
    max_corner.x = max(max_corner.x, vertex.pos.x);
    min_corner.x = min(min_corner.x, vertex.pos.x);
    max_corner.z = max(max_corner.z, vertex.pos.z);
    min_corner.z = min(min_corner.z, vertex.pos.z);
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

Entity load_gent(const char* path, Assets& assets, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  Entity out = {};
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  String source = platform::read_file_to_string(path, scratch_arena.allocator, error);
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
      out.pos = parse_vec3(value, error);
      out.rendered_pos = out.prev_pos = out.pos;
    }
    else if (key == "controlled_by_player")
    {
      out.controlled_by_player = parse_bool(value, error);
    }
    else if (key == "rotation")
    {
      out.rotation = parse_f32(value, error);
      out.rendered_rotation = out.prev_rotation = out.rotation;
    }
    else if (key == "target_rotation")
    {
      out.target_rotation = parse_f32(value, error);
    }
    else if (key == "velocity")
    {
      out.velocity = parse_vec3(value, error);
    }
    else if (key == "collidable")
    {
      out.collidable = parse_bool(value, error);
    }
    else if (key == "bounding_box")
    {
      // TODO: allow for defining 'bounding_box : *' before the mesh?
      if (value == "*")
      {
        ASSERT(out.renderable, "gent decoding error. Cannot calculate on non renderable entity.");
        out.bounding_box = BoundingBox::from_mesh(out.mesh, assets);
        out.is_bounding_box_from_model = true;
      }
      else
      {
        auto bb = parse_vec2(value, error);
        out.bounding_box.width = bb.x;
        out.bounding_box.depth = bb.y;
      }
    }
    else if (key == "renderable")
    {
      out.renderable = parse_bool(value, error);
    }
    else if (key == "mesh")
    {
      out.mesh_path = value.copy(allocator);
      out.mesh = assets.load_obj(out.mesh_path.data, allocator, error);
    }
    else if (key == "interactable")
    {
      out.interactable = parse_bool(value, error);
    }
    else if (key == "interactable_radius")
    {
      out.interactable_radius = parse_f32(value, error);
    }
    else if (key == "emits_light")
    {
      out.emits_light = parse_bool(value, error);
    }
    else if (key == "light_height_offset")
    {
      out.light_height_offset = parse_f32(value, error);
    }
    else if (key == "light_color")
    {
      out.light_color = parse_vec3(value, error);
    }
    else if (key == "tint")
    {
      out.tint = parse_vec3(value, error);
    }
    else if (key == "name")
    {
      out.name = value.copy(allocator);
    }
    ERROR_ASSERT(error == SUCCESS, out_error, error, out);
    // find-error on
  }
  return out;
}

Scene load_gscn(const char* path, Assets& assets, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  String source = platform::read_file_to_string(path, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});

  auto lines = source.split('\n', scratch_arena.allocator);
  Scene scene = {};
  auto entity_cache = Map<String, Entity>::make(lines.size * 2, scratch_arena.allocator);
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
      scene.ambient_color = parse_vec3(value, error);
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
      entity = load_gent(entity_path.to_cstr(scratch_arena.allocator), assets, allocator, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, scene);
      entity_cache.set(key, entity);
    }

    auto pos = parse_vec3(value, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, scene);

    entity.pos = pos;

    scene.entities[scene.entities_count++] = entity;
  }

  return scene;
}
