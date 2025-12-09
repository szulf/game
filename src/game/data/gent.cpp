#include "gent.h"

Entity entity_from_file(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  Entity entity = {};

  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));

  auto p = string_make(path);
  ERROR_ASSERT(
    string_starts_with_cstr(p, "data/") && string_ends_with_cstr(p, ".gent"),
    out_error,
    ENTITY_READ_ERROR_INVALID_PATH,
    entity
  );
  p.data += sizeof("data/") - 1;
  p.size -= sizeof("data/") - 1;
  p.size -= sizeof(".gent") - 1;
  entity.name = string_copy(p, allocator);

  usize file_size;
  void* file_ptr = platform.read_file(path, &scratch_arena.allocator, &file_size);
  auto file = string_make_len((const char*) file_ptr, file_size);

  auto lines = string_split(file, '\n', scratch_arena.allocator);
  for (usize line_idx = 0; line_idx < lines.size; ++line_idx)
  {
    auto& line = lines[line_idx];
    auto parts = string_split(line, ':', scratch_arena.allocator);
    ERROR_ASSERT(parts.size == 2, out_error, GLOBAL_ERROR_INVALID_DATA, entity);

    auto key = string_trim_whitespace(parts[0]);
    auto value = string_trim_whitespace(parts[1]);

    if (key == "position")
    {
      auto values = string_split(value, ' ', scratch_arena.allocator);
      ERROR_ASSERT(values.size == 3, out_error, ENTITY_READ_ERROR_INVALID_POSITION, entity);

      entity.position.x = string_parse_f32(values[0], error);
      ERROR_ASSERT(error == SUCCESS, out_error, ENTITY_READ_ERROR_INVALID_POSITION, entity);
      entity.position.y = string_parse_f32(values[1], error);
      ERROR_ASSERT(error == SUCCESS, out_error, ENTITY_READ_ERROR_INVALID_POSITION, entity);
      entity.position.z = string_parse_f32(values[2], error);
      ERROR_ASSERT(error == SUCCESS, out_error, ENTITY_READ_ERROR_INVALID_POSITION, entity);
    }
    else if (key == "model")
    {
      entity.has_model = true;
      entity.model_path = string_copy(value, allocator);
      entity.model = model_from_file(
        string_to_cstr(entity.model_path, scratch_arena.allocator),
        allocator,
        error
      );
      ERROR_ASSERT(error == SUCCESS, out_error, ENTITY_READ_ERROR_INVALID_MODEL, entity);
    }
    else if (key == "type")
    {
      entity.type = string_to_entity_type(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, ENTITY_READ_ERROR_INVALID_TYPE, entity);
    }
    else if (key == "interactable_type")
    {
      entity.interactable_type = string_to_interactable_type(value, error);
      ERROR_ASSERT(
        error == SUCCESS,
        out_error,
        ENTITY_READ_ERROR_INVALID_INTERACTABLE_TYPE,
        entity
      );
    }
    else if (key == "bounding_box")
    {
      if (value == "*")
      {
        ERROR_ASSERT(
          entity.has_model,
          out_error,
          ENTITY_READ_ERROR_DYNAMIC_BOUNDING_BOX_NO_MODEL,
          entity
        );
        entity.bounding_box = bounding_box_from_model(entity.model);
        entity.is_bounding_box_from_model = true;
      }
      else
      {
        ASSERT(false, "[TODO] load hardcoded bounding box");
      }
    }
    else
    {
      out_error = GLOBAL_ERROR_INVALID_DATA;
      return entity;
    }
  }
  return entity;
}

void entity_to_file(const Entity& entity)
{
  // TODO(szulf): i dont like this, i will surely exceed this at some point
  char buf[1024];
  int written = 0;

  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));

  written += fmt(
    buf + written,
    sizeof(buf) - (usize) written,
    "model : %s\n",
    string_to_cstr(entity.model_path, scratch_arena.allocator)
  );

  written += fmt(
    buf + written,
    sizeof(buf) - (usize) written,
    "type : %s\n",
    entity_type_to_cstr(entity.type)
  );

  if (entity.type == ENTITY_TYPE_INTERACTABLE)
  {
    written += fmt(
      buf + written,
      sizeof(buf) - (usize) written,
      "interactable_type : %s\n",
      interactable_type_to_cstr(entity.interactable_type)
    );
  }

  if (entity.is_bounding_box_from_model)
  {
    written += fmt(buf + written, sizeof(buf) - (usize) written, "bounding_box : %c\n", '*');
  }
  else
  {
    written += fmt(
      buf + written,
      sizeof(buf) - (usize) written,
      "bounding_box : %f %f\n",
      entity.bounding_box.width,
      entity.bounding_box.depth
    );
  }

  String str = string_make_len(buf, (usize) written);

  auto path = string_append_cstr(
    string_prepend_cstr(entity.name, "data/", scratch_arena.allocator),
    ".gent",
    scratch_arena.allocator
  );
  platform.write_file(string_to_cstr(path, scratch_arena.allocator), &str);
}
