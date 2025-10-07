#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;

typedef u32       b32;
typedef size_t    usize;
typedef uintptr_t ptrsize;

typedef float f32;
typedef double f64;

#define false 0
#define true 1

#define PI32 3.141592653f

#define S16_MAX  32767
#define S16_MIN -32768

#define FPS  60
#define MSPF (1000 / FPS)

#define TPS  20
#define MSPT (1000 / TPS)

#define KILOBYTES(n) ((n) * 1024)
#define MEGABYTES(n) (KILOBYTES(n) * 1024)
#define GIGBAYTES(n) (MEGABYTES(n) * 1024)

#define ARRAY_LENGTH(arr) (sizeof((arr)) / sizeof(*(arr)))

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

// TODO(szulf): need to implement this
static void log_(const char* file, usize line, const char* func, const char* fmt, ...);

#include "math.c"
#include "error.c"
#include "memory.c"
#include "array.h"
#include "string.c"

static void* platform_read_entire_file(Arena* arena, const char* path, Error* err);
static void* platform_read_entire_file_bytes_read(Arena* arena, const char* path,
                                                  usize* bytes_read, Error* err);

static void platform_print(const char* msg, ...);
// TODO(szulf): change to platform_get_ms?
static u64 get_ms();

// TODO(szulf): change to PlatformWindowDimensions?
typedef struct WindowDimensions
{
  s32 width;
  s32 height;
} WindowDimensions;

// TODO(szulf): change to platform_get_window_dimensions?
static WindowDimensions get_window_dimensions();

#include "image.c"
#include "renderer.c"

typedef struct GameSoundBuffer
{
  s16* memory;
  usize size;
  u32 sample_count;
  u32 samples_per_second;
} GameSoundBuffer;

typedef struct SceneArray
{
  usize cap;
  usize len;
  Scene* items;
} SceneArray;

typedef enum Key
{
  KEY_LMB,
  KEY_SPACE,
} Key;

// TODO(szulf): change to GameInputEvent?
typedef struct InputEvent
{
  Key key;
} InputEvent;

typedef struct InputEventArray
{
  usize cap;
  usize len;
  InputEvent* items;
} InputEventArray;

// TODO(szulf): change to GameAction?
typedef enum Action
{
  ACTION_CHANGE_SCENE,
  ACTION_MOVE,
} Action;

static Action keybind_map[] =
{
  [KEY_LMB] = ACTION_CHANGE_SCENE,
  [KEY_SPACE] = ACTION_MOVE,
};

typedef struct GameInput
{
  InputEventArray input_events;
} GameInput;

typedef struct GameState
{
  usize current_scene_idx;
  SceneArray scenes;
} GameState;

static void game_setup(Arena* perm_arena, Arena* temp_arena, GameState* state);

// TODO(szulf): need to interpolate the positions so the updates are not so sudden
static void game_update(GameState* state, GameInput* input);

static void game_render(GameState* state);

static void game_get_sound(GameSoundBuffer* sound_buffer);

#endif
