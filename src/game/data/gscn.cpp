#include "gscn.h"

namespace data
{

Array<Entity> scene_from_file(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));

  usize file_size;
  void* file_ptr = platform.read_file(path, &scratch_arena.allocator, &file_size, &error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  auto file = string_make_len((const char*) file_ptr, file_size);

  auto lines = string_split(file, '\n', scratch_arena.allocator);
  auto entities = array_make<Entity>(ARRAY_TYPE_STATIC, lines.size, allocator);
  auto entity_cache = map_make<String, Entity>(lines.size * 2, scratch_arena.allocator);
  for (usize line_idx = 0; line_idx < lines.size; ++line_idx)
  {
    Entity entity = {};
    auto& line = lines[line_idx];
    if (line[0] == '#')
    {
      continue;
    }

    auto parts = string_split(line, ':', scratch_arena.allocator);
    ERROR_ASSERT(parts.size == 2, out_error, GLOBAL_ERROR_INVALID_DATA, entities);

    auto entity_path_raw = string_trim_whitespace(parts[0]);
    if (map_contains(entity_cache, entity_path_raw))
    {
      entity = *map_get(entity_cache, entity_path_raw);
    }
    else
    {
      auto entity_path = string_append_cstr(
        string_prepend_cstr(entity_path_raw, "data/", scratch_arena.allocator),
        ".gent",
        scratch_arena.allocator
      );
      entity =
        entity_from_file(string_to_cstr(entity_path, scratch_arena.allocator), allocator, error);
      ERROR_ASSERT(error == SUCCESS, out_error, GLOBAL_ERROR_INVALID_DATA, entities);
      map_set(entity_cache, entity_path_raw, entity);
    }

    auto position_string = string_trim_whitespace(parts[1]);
    auto nums = string_split(position_string, ' ', scratch_arena.allocator);
    ERROR_ASSERT(nums.size == 3, out_error, GLOBAL_ERROR_INVALID_DATA, entities);

    f32 x = string_parse_f32(nums[0], error);
    ERROR_ASSERT(error == SUCCESS, out_error, GLOBAL_ERROR_INVALID_DATA, entities);
    f32 y = string_parse_f32(nums[1], error);
    ERROR_ASSERT(error == SUCCESS, out_error, GLOBAL_ERROR_INVALID_DATA, entities);
    f32 z = string_parse_f32(nums[2], error);
    ERROR_ASSERT(error == SUCCESS, out_error, GLOBAL_ERROR_INVALID_DATA, entities);

    entity.position = {x, y, z};

    array_push(entities, entity);
  }

  return entities;
}

void scene_to_file(const char* path, const Array<Entity>& entities, Error& out_error)
{
  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));

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
      string_to_cstr(entity.name, scratch_arena.allocator),
      entity.position.x,
      entity.position.y,
      entity.position.z
    );
  }
  auto gscn = string_make_len(buf, (usize) written);
  platform.write_file(path, &gscn, &error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
}

}
