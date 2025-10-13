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

#define false 0
#define true 1

#define PI32 3.141592653f

#define I16_MAX  32767
#define I16_MIN -32768

#define FPS  60
#define MSPF (1000 / FPS)

#define TPS  20
#define MSPT (1000 / TPS)

#define KILOBYTES(n) ((n) * 1024)
#define MEGABYTES(n) (KILOBYTES(n) * 1024)
#define GIGABYTES(n) (MEGABYTES(n) * 1024)

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
static void log_(const char* file, usize line, const char* func, const char* fmt, ...);

#include "math.c"
#include "error.h"
#include "memory.c"
#include "array.h"
#include "string.c"

static void* os_read_entire_file(const char* path, Arena* arena, Error* err);
static void* os_read_entire_file_bytes_read(const char* path, usize* bytes_read, Arena* arena,
                                            Error* err);

static void os_print(const char* msg);
static u64 os_get_ms(void);

typedef struct WindowDimensions
{
  i32 width;
  i32 height;
} WindowDimensions;

static WindowDimensions os_get_window_dimensions(void);

#include "image.c"
#include "renderer.c"

typedef struct SoundBuffer
{
  i16* memory;
  usize size;
  u32 sample_count;
  u32 samples_per_second;
} SoundBuffer;

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

typedef struct Input
{
  InputEventArray input_events;
} Input;

typedef struct State
{
  usize current_scene_idx;
  SceneArray scenes;
} State;

static void setup(State* state, Arena* temp_arena, Arena* perm_arena);

// TODO(szulf): need to interpolate the positions so the updates are not so sudden
static void update(State* state, Input* input);

static void render(State* state);

static void get_sound(SoundBuffer* sound_buffer);

#endif
