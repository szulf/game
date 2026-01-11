#include "gscn.h"

namespace data
{

Array<Entity> scene_from_file(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  usize file_size;
  void* file_ptr = platform::read_entire_file(path, scratch_arena.allocator, file_size, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  auto file = String::make((const char*) file_ptr, file_size);

  auto lines = file.split('\n', scratch_arena.allocator);
  auto entities = Array<Entity>::make(ArrayType::STATIC, lines.size, allocator);
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
    ERROR_ASSERT(parts.size == 2, out_error, GLOBAL_ERROR_INVALID_DATA, entities);

    auto key = parts[0].trim_whitespace();
    auto value = parts[1].trim_whitespace();

    if (key == "ambient_color")
    {
      ambient_color = get_vec3(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, entities);
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
      entity = entity_from_file(entity_path.to_cstr(scratch_arena.allocator), allocator, error);
      ERROR_ASSERT(error == SUCCESS, out_error, GLOBAL_ERROR_INVALID_DATA, entities);
      entity_cache.set(key, entity);
    }

    vec3 pos = get_vec3(value, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, entities);

    entity.pos = pos;

    entities.push(entity);
  }

  for (usize entity_idx = 0; entity_idx < entities.size; ++entity_idx)
  {
    auto& model = assets::model_get(entities[entity_idx].model);
    for (usize part_idx = 0; part_idx < model.parts.size; ++part_idx)
    {
      auto& material = assets::material_get(model.parts[part_idx].material);
      material.ambient_color = ambient_color;
    }
  }

  return entities;
}

void scene_to_file(const char* path, const Array<Entity>& entities, Error& out_error)
{
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  Error error = SUCCESS;
  // TODO(szulf): i dont like this, i will surely exceed this at some point
  char buf[1024];
  int written = 0;
  for (usize entity_idx = 0; entity_idx < entities.size; ++entity_idx)
  {
    auto& entity = entities[entity_idx];
    written += fmt(
      buf + written,
      sizeof(buf) - (usize) written,
      "%s : %f %f %f\n",
      entity.name.to_cstr(scratch_arena.allocator),
      entity.pos.x,
      entity.pos.y,
      entity.pos.z
    );
  }
  auto gscn = String::make(buf, (usize) written);
  platform::write_entire_file(path, gscn, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
}

}
