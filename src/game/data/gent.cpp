#include "gent.h"

namespace data
{

Entity entity_from_file(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  Entity entity = {};

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto p = String::make(path);
  ERROR_ASSERT(
    p.starts_with("data/") && p.ends_with(".gent"),
    out_error,
    ENTITY_READ_ERROR_INVALID_PATH,
    entity
  );
  p.data += sizeof("data/") - 1;
  p.size -= sizeof("data/") - 1;
  p.size -= sizeof(".gent") - 1;
  entity.name = p.copy(allocator);

  usize file_size;
  void* file_ptr = platform::read_entire_file(path, scratch_arena.allocator, file_size, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, entity);
  auto file = String::make((const char*) file_ptr, file_size);

  auto lines = file.split('\n', scratch_arena.allocator);
  for (usize line_idx = 0; line_idx < lines.size; ++line_idx)
  {
    auto& line = lines[line_idx];
    if (line[0] == '#')
    {
      continue;
    }

    auto parts = line.split(':', scratch_arena.allocator);
    ERROR_ASSERT(parts.size == 2, out_error, GLOBAL_ERROR_INVALID_DATA, entity);

    auto key = parts[0].trim_whitespace();
    auto value = parts[1].trim_whitespace();

    if (key == "position")
    {
      entity.position = get_vec3(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, entity);
    }
    else if (key == "model")
    {
      entity.has_model = true;
      entity.model_path = value.copy(allocator);
      entity.model = assets::model_from_file(
        entity.model_path.to_cstr(scratch_arena.allocator),
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
      ASSERT(
        entity.type == EntityType::INTERACTABLE,
        "cannot set interactable_type on non interactable entity"
      );
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
        entity.bounding_box = BoundingBox::from_model(entity.model);
        entity.is_bounding_box_from_model = true;
      }
      else
      {
        ASSERT(false, "[TODO] load hardcoded bounding box");
      }
    }
    else if (key == "tint")
    {
      entity.tint = get_vec3(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, entity);
    }
    else if (key == "light_bulb_color")
    {
      ASSERT(
        entity.type == EntityType::INTERACTABLE &&
          entity.interactable_type == InteractableType::LIGHT_BULB,
        "cannot set light_bulb_color on non light_bulb entity"
      );
      entity.light_bulb_color = get_vec3(value, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, entity);
    }
    else
    {
      out_error = GLOBAL_ERROR_INVALID_DATA;
      return entity;
    }
  }
  return entity;
}

void entity_to_file(const Entity& entity, Error& out_error)
{
  Error error = SUCCESS;
  // TODO(szulf): i dont like this, i will surely exceed this at some point
  char buf[1024];
  int written = 0;

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  written += fmt(
    buf + written,
    sizeof(buf) - (usize) written,
    "model : %s\n",
    entity.model_path.to_cstr(scratch_arena.allocator)
  );

  written += fmt(
    buf + written,
    sizeof(buf) - (usize) written,
    "type : %s\n",
    entity_type_to_cstr(entity.type)
  );

  if (entity.type == EntityType::INTERACTABLE)
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

  String str = String::make(buf, (usize) written);

  auto path =
    entity.name.prepend("data/", scratch_arena.allocator).append(".gent", scratch_arena.allocator);
  platform::write_entire_file(path.to_cstr(scratch_arena.allocator), str, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
}

}
