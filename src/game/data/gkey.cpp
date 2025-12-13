#include "gkey.h"

GameInput keymap_from_file(const char* path, Error& out_error)
{
  GameInput input = {};
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
    ERROR_ASSERT(parts.size == 2, out_error, GLOBAL_ERROR_INVALID_DATA, input);

    auto action = string_trim_whitespace(parts[0]);
    auto key_str = string_trim_whitespace(parts[1]);
    auto key = string_to_key(key_str, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, input);

    if (action == "move_front")
    {
      input.move_front.key = key;
    }
    else if (action == "move_back")
    {
      input.move_back.key = key;
    }
    else if (action == "move_left")
    {
      input.move_left.key = key;
    }
    else if (action == "move_right")
    {
      input.move_right.key = key;
    }
    else if (action == "interact")
    {
      input.interact.key = key;
    }
    else if (action == "toggle_camera_mode")
    {
      input.toggle_camera_mode.key = key;
    }
    else if (action == "toggle_display_bounding_boxes")
    {
      input.toggle_display_bounding_boxes.key = key;
    }
    else if (action == "camera_move_up")
    {
      input.camera_move_up.key = key;
    }
    else if (action == "camera_move_down")
    {
      input.camera_move_down.key = key;
    }
    else
    {
      out_error = GLOBAL_ERROR_INVALID_DATA;
      return input;
    }
  }

  return input;
}

void keymap_to_file(const char* path, GameInput& input)
{
#define WRITE_KEY(key_state)                                                                       \
  do                                                                                               \
  {                                                                                                \
    written += fmt(                                                                                \
      buf + written,                                                                               \
      sizeof(buf) - (usize) written,                                                               \
      #key_state " : %s\n",                                                                        \
      key_to_cstr(input.key_state.key)                                                             \
    );                                                                                             \
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
