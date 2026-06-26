#include <cstdint>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <memory>
#include <any>
#include <functional>
#include <print>
#include <bitset>
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

// TODO: add a way to add messages to asserts
// and change all NOTE's near the asserts to the messages
#define ASSERT(expr)                                                                               \
  do {                                                                                             \
    if (!(expr)) {                                                                                 \
      std::println("Assertion failed on expression: '{}'", #expr);                                 \
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

struct ivec2 {
  i32 x{};
  i32 y{};
};

ivec2 operator+(const ivec2& a, const ivec2& b) {
  return {a.x + b.x, a.y + b.y};
}

ivec2 operator-(const ivec2& a, const ivec2& b) {
  return {a.x - b.x, a.y - b.y};
}

// TODO: should i keep this?
ivec2 operator*(const ivec2& a, f32 scalar) {
  return {i32(f32(a.x * scalar)), i32(f32(a.y * scalar))};
}

// TODO: should i keep this?
ivec2 operator*(const ivec2& a, i32 scalar) {
  return {a.x * scalar, a.y * scalar};
}

ivec2 operator*(const ivec2& a, const ivec2& b) {
  return {a.x * b.x, a.y * b.y};
}

ivec2 operator/(const ivec2& a, i32 b) {
  return {a.x / b, a.y / b};
}

ivec2 operator/(const ivec2& a, const ivec2& b) {
  return {a.x / b.x, a.y / b.y};
}

ivec2& operator+=(ivec2& a, const ivec2& b) {
  a.x += b.x;
  a.y += b.y;
  return a;
}

bool operator==(const ivec2& a, const ivec2& b) {
  return a.x == b.x && a.y == b.y;
}

i32 length2(const ivec2& v) {
  return (v.x * v.x) + (v.y * v.y);
}

ivec2 ivec2_from_vector2(const Vector2& vec) {
  return {i32(vec.x), i32(vec.y)};
}

Vector2 vector2_from_ivec2(const ivec2& vec) {
  return {f32(vec.x), f32(vec.y)};
}

enum class Direction {
  Up,
  Right,
  Down,
  Left,
  Count,
};

Direction opposite_direction(Direction direction) {
  switch (direction) {
    case Direction::Up:
      return Direction::Down;
    case Direction::Down:
      return Direction::Up;
    case Direction::Right:
      return Direction::Left;
    case Direction::Left:
      return Direction::Right;
    case Direction::Count:
      break;
  }
  ASSERT(false);
}

ivec2 direction_to_ivec2(Direction direction) {
  switch (direction) {
    case Direction::Up:
      return {0, -1};
    case Direction::Down:
      return {0, 1};
    case Direction::Right:
      return {1, 0};
    case Direction::Left:
      return {-1, 0};
    case Direction::Count:
      break;
  }
  ASSERT(false);
}

using Rotation = Direction;

// TODO: not sure if right and left degrees are correct
f32 rotation_degrees(Rotation rotation) {
  switch (rotation) {
    case Rotation::Up:
      return 0;
    case Rotation::Down:
      return 180;
    case Rotation::Right:
      return 90;
    case Rotation::Left:
      return 270;
    case Rotation::Count:
      break;
  }
  ASSERT(false);
}

#include "ui.h"
#include "items.h"
#include "entity.h"
#include "game.h"

#include "ui.cpp"
#include "entity.cpp"
#include "systems.cpp"
#include "game.cpp"

static constexpr i32 TPS = 60;
static constexpr f32 DT  = 1.0f / TPS;

int main() {
  State state = {};
  init(state);

  f64 current_time = GetTime();
  f64 accumulator  = 0;

  while (!WindowShouldClose()) {
    f64 new_time   = GetTime();
    f64 frame_time = new_time - current_time;
    current_time   = new_time;
    accumulator += frame_time;

    gather_input(state);

    while (accumulator >= DT) {
      update_tick(state, DT);
      accumulator -= DT;
    }

    update_frame(state);
    render(state);
  }

  shutdown(state);

  return 0;
}
