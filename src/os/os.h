#ifndef OS_H
#define OS_H

#include "base/base.h"
#include "base/string.h"
#include "base/enum_array.h"

namespace os
{

void init();

enum class Key
{
  A = 1,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  SPACE,
  LSHIFT,

  COUNT
};

const char* key_to_cstr(Key key);
Key string_to_key(const String& str, Error& out_error);

struct KeyState
{
  u32 transition_count;
  bool ended_down;
};

struct Input
{
  inline KeyState& operator[](Key key)
  {
    return states[key];
  }

  enum_array<Key, KeyState> states;

  vec2 mouse_pos;
  vec2 mouse_delta;
  vec2 mouse_pos_last;
};

struct Window
{
  struct Dimensions
  {
    u32 width;
    u32 height;
  };

  static Window open(const char* name, Dimensions dimensions);
  void init_rendering_api();
  void update();
  void clear_input();
  void swap_buffers();

  // NOTE: needs to be called every frame to properly consume the pointer
  void consume_mouse_pointer();
  void release_mouse_pointer();

  const char* name;
  Dimensions dimensions;

  bool running;
  Input input;

  void* handle;
};

void* read(const char* path, Allocator& allocator, usize& out_size, Error& out_error);
String read_to_string(const char* path, Allocator& allocator, Error& out_error);

f32 time_now();

void* alloc(usize bytes);
void free(void* ptr);

}

#endif
