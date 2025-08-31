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

typedef s32       bool32;
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

static void log_(const char* file, usize line, const char* func, const char* fmt, ...);

#include "math.c"
#include "error.c"
#include "memory.c"
#include "array.h"
#include "string.c"

static Error platform_read_entire_file(void** out, Arena* arena, const char* path);
static Error platform_read_entire_file_bytes_read(void** out, Arena* arena, const char* path,
                                                  usize* bytes_read);

static void print(const char* msg);
static u64 get_ms();

typedef struct WindowDimensions
{
  s32 width;
  s32 height;
} WindowDimensions;

static WindowDimensions get_window_dimensions();

// TODO(szulf): this will probably need to change to a renderer.cpp
#include "renderer.h"

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

typedef struct GameState
{
  usize current_scene_idx;
  SceneArray scenes;
} GameState;

static Error game_setup(Arena* perm_arena, Arena* temp_arena, GameState* state);

static void game_update(GameState* state);

static void game_render(GameState* state);

static void game_get_sound(GameSoundBuffer* sound_buffer);

#endif
