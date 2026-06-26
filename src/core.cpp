#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <print>
#include <vector>
#include <limits>
#include <algorithm>

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
      printf("Assertion failed on expression: '%s'\n", #expr);                                     \
      abort();                                                                                     \
    }                                                                                              \
  } while (false)

#define ASSERT(expr, ...)                                                                          \
  do {                                                                                             \
    if (!(expr)) {                                                                                 \
      printf("Assertion failed on expression: '%s' with message:\n", #expr);                       \
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

ivec2 direction_to_ivec2(Direction direction) {
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

static constexpr ivec2 WINDOW_DIMS = {1280, 720};
static constexpr ivec2 GRID_DIMS   = {32, 32};

static constexpr i32 TPS = 60;
static constexpr f32 DT  = 1.0f / TPS;
