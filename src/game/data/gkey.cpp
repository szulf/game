#include "gkey.h"

namespace data
{

game::Input keymap_from_file(const char* path, Error& out_error)
{
  game::Input input = {};
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  usize file_size;
  void* file_ptr = platform::read_entire_file(path, scratch_arena.allocator, file_size, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, input);
  auto file = String::make((const char*) file_ptr, file_size);

  auto lines = file.split('\n', scratch_arena.allocator);
  for (usize line_idx = 0; line_idx < lines.size; ++line_idx)
  {
    auto& line = lines[line_idx];
    auto parts = line.split(':', scratch_arena.allocator);
    ERROR_ASSERT(parts.size == 2, out_error, GLOBAL_ERROR_INVALID_DATA, input);

    auto action = parts[0].trim_whitespace();
    auto key_str = parts[1].trim_whitespace();
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

void keymap_to_file(const char* path, game::Input& input, Error& out_error)
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

  Error error = SUCCESS;
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

  auto s = String::make(buf, (usize) written);
  platform::write_entire_file(path, s, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
}

}
