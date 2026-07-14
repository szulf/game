#include <cstdint>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <print>
#include <vector>
#include <limits>
#include <algorithm>
#include <bitset>

#include "raylib.h"
#include "raymath.h"

using i32 = int32_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

static constexpr u16 U16_MAX = std::numeric_limits<u16>::max();

#define ASSERT_NO_MSG(expr)                                                                        \
  do {                                                                                             \
    if (!(expr)) {                                                                                 \
      printf("Assertion (%s:%d) failed on expression: '%s'\n", __FILE__, __LINE__, #expr);         \
      abort();                                                                                     \
    }                                                                                              \
  } while (false)

#define ASSERT(expr, ...)                                                                          \
  do {                                                                                             \
    if (!(expr)) {                                                                                 \
      printf(                                                                                      \
        "Assertion (%s:%d) failed on expression: '%s' with message:\n",                            \
        __FILE__,                                                                                  \
        __LINE__,                                                                                  \
        #expr                                                                                      \
      );                                                                                           \
      printf(__VA_ARGS__);                                                                         \
      printf("\n");                                                                                \
      abort();                                                                                     \
    }                                                                                              \
  } while (false)

// NOTE: raylib doesnt have this
bool operator==(const Color& a, const Color& b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// NOTE: for std::visit
template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#include "math.cpp"

ivec2 ivec2_from_vector2(const Vector2& vec) {
  return {i32(vec.x), i32(vec.y)};
}

Vector2 vector2_from_ivec2(const ivec2& vec) {
  return {f32(vec.x), f32(vec.y)};
}

vec2 vec2_from_vector2(const Vector2& vec) {
  return {vec.x, vec.y};
}

Vector2 vector2_from_vec2(const vec2& vec) {
  return {vec.x, vec.y};
}

Rectangle rect_from_vec2x2(const vec2& pos, const vec2& dims) {
  return {
    .x      = pos.x,
    .y      = pos.y,
    .width  = dims.x,
    .height = dims.y,
  };
}

vec2 pos_from_rect(const Rectangle& rect) {
  return {rect.x, rect.y};
}

vec2 dims_from_rect(const Rectangle& rect) {
  return {rect.width, rect.height};
}

vec2 dims_from_texture(const Texture2D& texture) {
  return {f32(texture.width), f32(texture.height)};
}

enum class Direction {
  UP,
  RIGHT,
  DOWN,
  LEFT,
  COUNT,
};

Direction opposite_direction(Direction direction) {
  switch (direction) {
    case Direction::UP:
      return Direction::DOWN;
    case Direction::DOWN:
      return Direction::UP;
    case Direction::RIGHT:
      return Direction::LEFT;
    case Direction::LEFT:
      return Direction::RIGHT;
    case Direction::COUNT:
      break;
  }
  ASSERT(false, "invalid direction: %d\n", i32(direction));
}

vec2 direction_to_vec2(Direction direction) {
  switch (direction) {
    case Direction::UP:
      return {0, -1};
    case Direction::DOWN:
      return {0, 1};
    case Direction::RIGHT:
      return {1, 0};
    case Direction::LEFT:
      return {-1, 0};
    case Direction::COUNT:
      break;
  }
  ASSERT(false, "invalid direction: %d\n", i32(direction));
}

using Rotation = Direction;

// TODO: not sure if right and left degrees are correct
f32 rotation_degrees(Rotation rotation) {
  switch (rotation) {
    case Rotation::UP:
      return 0;
    case Rotation::DOWN:
      return 180;
    case Rotation::RIGHT:
      return 90;
    case Rotation::LEFT:
      return 270;
    case Rotation::COUNT:
      break;
  }
  ASSERT(false, "invalid rotation: %d\n", i32(rotation));
}

static constexpr vec2 WINDOW_DIMS = {1280, 720};
static constexpr vec2 GRID_DIMS   = {32, 32};

static constexpr i32 TPS = 60;
static constexpr f32 DT  = 1.0f / TPS;

// TODO: seed it always in the same way in debug builds?
inline std::mt19937 random_generate() {
  std::random_device rd{};
  std::seed_seq ss{rd()};
  return std::mt19937{ss};
}

inline std::mt19937 g_random_mt = random_generate();

template <typename T>
inline T random_get(T min, T max) {
  return std::uniform_int_distribution<T>{min, max}(g_random_mt);
}
