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

#define PI32 3.141592653f

#define I16_MAX  32767
#define I16_MIN -32768

#define FPS  60
#define MSPF (1000 / FPS)

#define TPS  20
#define MSPT (1000 / TPS)

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
    LOG(__VA_ARGS__); \
    __asm__("int3"); \
  } \
} while (0)
#else
#  define ASSERT(expr, msg)
#endif

// TODO(szulf): need to implement this
template <typename... Args>
static void log_(const char* file, usize line, const char* func, const char* fmt, Args... args);

#include "math.cpp"
#include "error.h"
#include "memory.cpp"
#include "array.cpp"
#include "string.cpp"

static void* os_read_entire_file(const char* path, Arena* arena, Error* err,
                                 usize* bytes_read = 0);
static void os_print(const char* msg);
static u64 os_get_ms(void);

struct WindowDimensions
{
  i32 width;
  i32 height;
};
static WindowDimensions os_get_window_dimensions(void);

#include "image.cpp"
#include "renderer.cpp"
#include "assets.cpp"
#include "obj.cpp"

struct SoundBuffer
{
  i16* memory;
  usize size;
  u32 sample_count;
  u32 samples_per_second;
};

enum Key
{
  KEY_LMB,
  KEY_SPACE,
};

struct InputEvent
{
  Key key;
};

enum Action
{
  ACTION_CHANGE_SCENE,
  // NOTE(szulf): depending on this being last
  ACTION_MOVE,
};

static Action keybind_map[ACTION_MOVE + 1];
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

static void setup(State* state, Arena* temp_arena, Arena* perm_arena);

// TODO(szulf): need to interpolate the positions so the updates are not so sudden
static void update(State* state, Input* input);

static void render(State* state);

static void get_sound(SoundBuffer* sound_buffer);

#endif
