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
#define MEGABYTES(n) (KILOBYTES(n) * 1024)
#define GIGBAYTES(n) (MEGABYTES(n) * 1024)

#define LOG(msg, ...) log_(__FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)

#ifdef GAME_DEBUG
#  define ASSERT(expr, msg, ...) do \
{ \
  if (!(expr)) \
  { \
    LOG(msg, ##__VA_ARGS__); \
    __asm__("int3"); \
  } \
} while (0)
#else
#  define ASSERT(expr, msg)
#endif

#define TODO(msg) ASSERT(false, "[TODO] " msg)
#define UNUSED(var) (void) var;

// TODO(szulf): need to implement this
static void log_(const char* file, usize line, const char* func, const char* fmt, ...);

#include "math.cpp"
#include "error.cpp"
#include "memory.cpp"
#include "array.cpp"
#include "string.cpp"

static void* platform_read_entire_file(const char* path, Arena* arena, Error* err,
                                       usize* bytes_read = nullptr);

static void platform_print(const char* msg);
static u64 platform_get_ms();

struct PlatformWindowDimensions
{
  i32 width;
  i32 height;
};

static PlatformWindowDimensions platform_get_window_dimensions();

#include "image.cpp"
#include "renderer.cpp"

struct SoundBuffer
{
  i16* memory;
  usize size;
  u32 sample_count;
  u32 samples_per_second;
};

enum class Key : u8
{
  Space,
  // NOTE(szulf): this has to be last?
  LMB,
};

struct InputEvent
{
  Key key;
};

enum class Action : u8
{
  ChangeScene,
  Move,
};

static Action g_keybind_map[(usize) Key::LMB + 1];
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

static void game_setup(Arena* perm_arena, Arena* temp_arena, State* state);

// TODO(szulf): need to interpolate the positions so the updates are not so sudden
static void game_update(State* state, Input* input);

static void game_render(State* state);

static void game_get_sound(SoundBuffer* sound_buffer);

#endif
