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

    auto entity_path_raw = parts[0].trim_whitespace();
    if (entity_cache.contains(entity_path_raw))
    {
      entity = *entity_cache[entity_path_raw];
    }
    else
    {
      auto entity_path = entity_path_raw.prepend("data/", scratch_arena.allocator)
                           .append(".gent", scratch_arena.allocator);
      entity = entity_from_file(entity_path.to_cstr(scratch_arena.allocator), allocator, error);
      ERROR_ASSERT(error == SUCCESS, out_error, GLOBAL_ERROR_INVALID_DATA, entities);
      entity_cache.set(entity_path_raw, entity);
    }

    auto position_string = parts[1].trim_whitespace();
    vec3 pos = get_vec3(position_string, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, entities);

    entity.position = pos;

    entities.push(entity);
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
      entity.position.x,
      entity.position.y,
      entity.position.z
    );
  }
  auto gscn = String::make(buf, (usize) written);
  platform::write_entire_file(path, gscn, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
}

}
