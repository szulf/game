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

using f32 = float;

constexpr f32 PI32 = 3.141592653f;

constexpr i16 I16_MAX = 0x7FFF;
constexpr i16 I16_MIN = -0x8000;

constexpr u8 FPS = 60;
constexpr u8 MSPF = 1000 / FPS;

constexpr u8 TPS = 20;
constexpr u8 MSPT = 1000 / 20;

#define LOG(...) log_(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define UNUSED(var) (void) (var)
#define TODO(...) ASSERT(false, __VA_ARGS__)

#ifdef GAME_DEBUG
#  define ASSERT(expr, ...)                                                                        \
    do                                                                                             \
    {                                                                                              \
      if (!(expr))                                                                                 \
      {                                                                                            \
        LOG("Assertion failed on expression: '{}' with message:", #expr);                          \
        LOG(__VA_ARGS__);                                                                          \
        asm("int3");                                                                               \
      }                                                                                            \
    }                                                                                              \
    while (0)
#else
#  define ASSERT(expr, msg)
#endif

template <typename... Args>
static void
log_(const char* file, usize line, const char* func, const char* fmt, const Args&... args);

#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <ranges>
#include <print>
#include <cstring>
#include <optional>

#include "math.cpp"

static std::pair<i32, i32> get_window_dimensions();

#include "png.cpp"
#include "renderer.cpp"
#include "assets.cpp"
#include "obj.cpp"

namespace game
{

class SoundBuffer
{
public:
  SoundBuffer() {}

public:
  i16* memory;
  usize size;
  u32 sample_count;
  u32 samples_per_second;
};

enum class Key : u8
{
  Return,
  Escape,
  Backspace,
  Tab,
  Lmb,
  Zero,
  One,
  Two,
  Three,
  Four,
  Five,
  Six,
  Seven,
  Eight,
  Nine,
  Semicolon,
  A,
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
  Space,
  LastKey,
};

struct InputEvent
{
  Key key;
};

enum class Action : u8
{
  ChangeScene,
  // NOTE(szulf): depending on this being last
  Move,
};

class KeybindMap
{
public:
  KeybindMap()
  {
    map[static_cast<usize>(Key::Lmb)] = Action::ChangeScene;
    map[static_cast<usize>(Key::Space)] = Action::Move;
  }

  inline Action&
  operator[](Key key)
  {
    ASSERT(key != Key::LastKey, "last key value cannot be used");
    return map[static_cast<usize>(key)];
  }

  inline Action
  operator[](Key key) const
  {
    ASSERT(key != Key::LastKey, "last key value cannot be used");
    return map[static_cast<usize>(key)];
  }

private:
  Action map[static_cast<usize>(Key::LastKey)]{};
};

struct Input
{
  std::vector<InputEvent> input_events;
};

class Game
{
public:
  Game();

  // TODO(szulf): need to interpolate the positions so the updates are not so sudden
  void update(Input& input);
  void update_frame();
  void render();
  void get_sound(SoundBuffer& sound_buffer);

  Game(const Game& other) = delete;
  Game& operator=(const Game& other) = delete;
  Game(Game&& other) = delete;
  Game& operator=(Game&& other) = delete;

private:
  usize current_scene_idx{};
  std::vector<Scene> scenes{};
  ShaderMap shader_map{};
  KeybindMap keybind_map{};
  Renderer renderer{shader_map};
  SoundBuffer sound_buffer{};
};

}

#endif
