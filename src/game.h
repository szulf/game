#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include <cstddef>

using u8 = std::uint8_t;
using i8 = std::int8_t;
using u16 = std::uint16_t;
using i16 = std::int16_t;
using u32 = std::uint32_t;
using i32 = std::int32_t;
using u64 = std::uint64_t;
using i64 = std::int64_t;

using usize = std::size_t;
using ptrsize = std::ptrdiff_t;

using f32 = float;
using f64 = double;

static constexpr f32 PI32 = 3.141592653f;

static constexpr i16 I16_MAX = 32767;
static constexpr i16 I16_MIN = -32768;

static constexpr i32 FPS = 60;
static constexpr i32 MSPF = 1000 / FPS;

static constexpr i32 TPS = 20;
static constexpr i32 MSPT = 1000 / TPS;

consteval u64 kilobytes(u64 n);
consteval u64 megabytes(u64 n);
consteval u64 gigabytes(u64 n);

// #define LOG(msg, ...) log_(__FILE__, __LINE__, __func__, msg, ##__VA_ARGS__)
#define LOG(msg, ...)

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
#define UNUSED(var) (void) var

#include <iostream>
#include <charconv>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <memory_resource>
#include <sstream>

#include "math.cpp"
#include "error.cpp"
#include "memory.cpp"

namespace platform
{

static AllocatedBuffer read_entire_file(const char* path, Error* err, usize* bytes_read = nullptr);

static void print(const char* msg);
static u64 get_ms();

struct WindowDimensions
{
  i32 width;
  i32 height;
};

static WindowDimensions get_window_dimensions();

}

#include "image.cpp"
#include "renderer.cpp"

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
  std::pmr::vector<InputEvent> input_events;
};

struct State
{
  usize current_scene_idx;
  std::pmr::vector<Scene> scenes;
};

static void setup(State& state);

// TODO(szulf): need to interpolate the positions so the updates are not so sudden
static void update(State& state, Input& input);

static void render(State& state);

static void get_sound(SoundBuffer& sound_buffer);

}

#endif
