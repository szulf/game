#include "gkey.h"

GameKeymap keymap_from_file(const char* path, Error& out_error)
{
  GameKeymap key_map = {};
  Error error = SUCCESS;
  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));

  usize file_size;
  void* file_ptr = platform.read_file(path, &scratch_arena.allocator, &file_size);
  auto file = string_make_len((const char*) file_ptr, file_size);

  auto lines = string_split(file, '\n', scratch_arena.allocator);
  for (usize line_idx = 0; line_idx < lines.size; ++line_idx)
  {
    auto& line = lines[line_idx];
    auto parts = string_split(line, ':', scratch_arena.allocator);
    ERROR_ASSERT(parts.size == 2, out_error, GLOBAL_ERROR_INVALID_DATA, key_map);

    auto action = string_trim_whitespace(parts[0]);
    auto key_str = string_trim_whitespace(parts[1]);
    auto key = string_to_key(key_str, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, key_map);

    if (action == "move_front")
    {
      key_map.move_front = key;
    }
    else if (action == "move_back")
    {
      key_map.move_back = key;
    }
    else if (action == "move_left")
    {
      key_map.move_left = key;
    }
    else if (action == "move_right")
    {
      key_map.move_right = key;
    }
    else if (action == "interact")
    {
      key_map.interact = key;
    }
    else if (action == "toggle_camera_mode")
    {
      key_map.toggle_camera_mode = key;
    }
    else if (action == "toggle_display_bounding_boxes")
    {
      key_map.toggle_display_bounding_boxes = key;
    }
    else
    {
      out_error = GLOBAL_ERROR_INVALID_DATA;
      return key_map;
    }
  }

  return key_map;
}

void keymap_to_file(const char* path, GameKeymap& key_map)
{
#define WRITE_KEY(key)                                                                             \
  do                                                                                               \
  {                                                                                                \
    written +=                                                                                     \
      fmt(buf + written, sizeof(buf) - (usize) written, #key " : %s\n", key_to_cstr(key_map.key)); \
  }                                                                                                \
  while (false)

  // TODO(szulf): i dont like this, will definitely exceed this limit
  char buf[1024];
  int written = 0;

  WRITE_KEY(move_front);
  WRITE_KEY(move_back);
  WRITE_KEY(move_left);
  WRITE_KEY(move_right);
  WRITE_KEY(interact);
  WRITE_KEY(toggle_camera_mode);
  WRITE_KEY(toggle_display_bounding_boxes);

  auto s = string_make_len(buf, (usize) written);
  platform.write_file(path, &s);
}
