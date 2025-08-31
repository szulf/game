#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stddef.h>

using u8  = uint8_t;
using i8  = int8_t;
using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;
using i64 = int64_t;

using bool32  = i32;
using usize   = size_t;
using ptrsize = uintptr_t;

using f32 = float;
using f64 = double;

#define PI32 3.141592653f

#define I16_MAX  32767
#define I16_MIN -32768

#define FPS  60
#define MSPF (1000 / FPS)

#define TPS  20
#define MSPT (1000 / TPS)

#define KILOBYTES(n) ((n) * 1024)
#define MEGABYTES(n) (KILOBYTES(n) * 1024)
#define GIGBAYTES(n) (MEGABYTES(n) * 1024)

#define LOG(msg, ...) log_(__FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)

#ifdef GAME_DEBUG
#  include <stdlib.h>
#  define ASSERT(expr, msg, ...) do \
{ \
  if (!(expr)) \
  { \
    LOG(msg, ##__VA_ARGS__); \
    asm ("int3"); \
  } \
} while (0)
#else
#  define ASSERT(expr, msg)
#endif

template <typename... Args>
static void log_(const char* file, i64 line, const char* func, const char* fmt, const Args&... args);

#include "math.cpp"
#include "result.cpp"
#include "memory.cpp"
#include "array.cpp"
#include "string.cpp"

template <typename... Args>
static void format(String& buf, const char* fmt, const Args&... args);

namespace platform
{
  static Result<void*> read_entire_file(mem::Arena& arena, const char* path,
                                        usize* bytes_read = nullptr);
  // TODO(szulf): cannot be static because compiler complains idk why
  void print(const char* msg);
  static u64 get_ms();

  struct WindowDimensions
  {
    i32 width;
    i32 height;
  };
  
  static WindowDimensions get_window_dimensions();
}

#include "vec3.cpp"
#include "vec4.cpp"
#include "mat4.cpp"

// TODO(szulf): this will probably need to change to a renderer.cpp
#include "renderer.h"

namespace game
{

struct SoundBuffer
{
  i16* memory;
  usize size;
  u32 sample_count;
  u32 samples_per_second;
};

struct State
{
  usize current_scene_idx;
  Array<Scene> scenes;
};

static void setup(mem::Arena& perm_arena, mem::Arena& temp_arena, State& state);

static void render(State& state);

static void get_sound(SoundBuffer& sound_buffer);

}


#endif
