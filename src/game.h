#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef int8_t   i8;
typedef uint16_t u16;
typedef int16_t  i16;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint64_t u64;
typedef int64_t  i64;

typedef u32       b32;
typedef size_t    usize;
typedef uintptr_t ptrsize;

typedef float f32;
typedef double f64;

constexpr f32 PI32 = 3.141592653f;

constexpr i16 I16_MAX = 0x7FFF;
constexpr i16 I16_MIN = -0x8000;

constexpr u8 FPS = 60;
constexpr u8 MSPF = 1000 / FPS;

constexpr u8 TPS = 20;
constexpr u8 MSPT = 1000 / 20;

#define KILOBYTES(n) ((n) * 1024)
#define MEGABYTES(n) (KILOBYTES(n) * 1024ll)
#define GIGABYTES(n) (MEGABYTES(n) * 1024ll)

#define LOG(...) log_(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define UNUSED(var) (void) (var)
#define TODO(...) ASSERT(false, __VA_ARGS__)

#ifdef GAME_DEBUG
#  define ASSERT(expr, ...) do \
{ \
  if (!(expr)) \
  { \
    LOG("Assertion failed on expression: '{}' with message:", #expr); \
    LOG(__VA_ARGS__); \
    __asm__("int3"); \
  } \
} while (0)
#else
#  define ASSERT(expr, msg)
#endif

template <typename... Args>
static void log_(const char* file, usize line, const char* func, const char* fmt,
                 const Args&... args);

#include "math.cpp"
#include "error.h"
#include "memory.cpp"
#include "array.cpp"
#include "string.cpp"

namespace os
{

static void* read_entire_file(const char* path, mem::Arena& arena, Error* err,
                              usize* bytes_read = nullptr);
static void print(const char* msg);
static u64 get_ms();

struct WindowDimensions
{
  i32 width;
  i32 height;
};
static WindowDimensions get_window_dimensions();

}

#include "png.cpp"
#include "renderer.cpp"
#include "assets.cpp"
#include "obj.cpp"

namespace game
{

struct SoundBuffer
{
  i16* memory;
  usize size;
  u32 sample_count;
  u32 samples_per_second;
};

enum class Key : u8
{
  LMB,
  SPACE,
};

struct InputEvent
{
  Key key;
};

enum class Action : u8
{
  CHANGE_SCENE,
  // NOTE(szulf): depending on this being last
  MOVE,
};

static Action keybind_map[(usize) Action::MOVE + 1];
static void setup_default_keybinds();

struct Input
{
  Array<InputEvent> input_events;
};

struct State
{
  usize current_scene_idx;
  Array<Scene> scenes;
};

static void setup(State& state, mem::Arena& temp_arena, mem::Arena& perm_arena);

// TODO(szulf): need to interpolate the positions so the updates are not so sudden
static void update(State& state, Input& input);

static void render(State& state);

static void get_sound(SoundBuffer& sound_buffer);

}

#endif
