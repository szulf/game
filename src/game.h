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

#ifdef GAME_DEBUG
#  include <stdlib.h>
// TODO(szulf): log a message in assert
#  define ASSERT(expr, msg) do \
{ \
  if (!(expr)) \
  { \
    exit(1); \
  } \
} while (0)
#else
#  define ASSERT(expr, msg)
#endif

#include "math.cpp"
#include "result.cpp"
#include "memory.cpp"
#include "array.cpp"

namespace platform
{
  static Result<void*> read_entire_file(mem::Arena& arena, const char* path,
                                            usize* bytes_read = nullptr);
  static u64 get_ms();
}


#include "vec3.cpp"
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
  Model model;
};

static void setup(mem::Arena& arena, State& state);

static void render(State& state);

static void get_sound(SoundBuffer& sound_buffer);

}


#endif
