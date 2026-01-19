#ifndef PLATFORM_H
#define PLATFORM_H

#include "gl_functions.h"

static u32 FPS = 165;
#define MSPF (1000 / FPS)

namespace platform
{

void* read_entire_file(const char* path, Allocator& allocator, usize& out_size, Error& out_error);
String read_file_to_string(const char* path, Allocator& allocator, Error& out_error);
void write_entire_file(const char* path, const String& string, Error& out_error);
u32 get_width();
u32 get_height();
u64 get_ns();

}

namespace game
{

struct Memory
{
  void* memory;
  usize size;
};

struct Spec
{
  const char* window_name;
  u32 width;
  u32 height;
  usize memory_size;
};

struct KeyState
{
  Key key;
  u32 transition_count;
  bool ended_down;
};

struct Input
{
  union
  {
    KeyState states[9];
    struct
    {
      KeyState move_front;
      KeyState move_back;
      KeyState move_left;
      KeyState move_right;
      KeyState interact;

      KeyState camera_move_up;
      KeyState camera_move_down;
      KeyState toggle_camera_mode;
      KeyState toggle_display_bounding_boxes;
    };
  };

  vec2 mouse_pos;
  vec2 mouse_relative;
  vec2 mouse_pos_last;
};

void spec(Spec& spec);
void init(Memory& memory, Input& input);
void update(Memory& memory, Input& input, float dt);
void render(Memory& memory);
void event(Memory& memory);

}

#endif
