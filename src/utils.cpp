#include "utils.h"

bool operator==(const Color& a, const Color& b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

vec2 vec2_from_raylib(const Vector2& vec) {
  return {vec.x, vec.y};
}

Vector2 vec2_to_raylib(const vec2& vec) {
  return {vec.x, vec.y};
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

// TODO: could possibly do some bit shifting shit, but this seems easier for now
Direction next_direction(Direction direction) {
  switch (direction) {
    case DIR_UP:
      return DIR_RIGHT;
    case DIR_RIGHT:
      return DIR_DOWN;
    case DIR_DOWN:
      return DIR_LEFT;
    case DIR_LEFT:
      return DIR_UP;
  }
  ASSERT(false, "invalid direction: {}\n", i32(direction));
}

Direction opposite_direction(Direction direction) {
  switch (direction) {
    case DIR_UP:
      return DIR_DOWN;
    case DIR_DOWN:
      return DIR_UP;
    case DIR_RIGHT:
      return DIR_LEFT;
    case DIR_LEFT:
      return DIR_RIGHT;
  }
  ASSERT(false, "invalid direction: {}\n", i32(direction));
}

vec2 direction_to_vec2(Direction direction) {
  switch (direction) {
    case DIR_UP:
      return {0, -1};
    case DIR_DOWN:
      return {0, 1};
    case DIR_RIGHT:
      return {1, 0};
    case DIR_LEFT:
      return {-1, 0};
  }
  ASSERT(false, "invalid direction: {}\n", i32(direction));
}

std::string_view direction_to_string(Direction direction) {
  switch (direction) {
    case DIR_UP:
      return "up";
    case DIR_DOWN:
      return "down";
    case DIR_RIGHT:
      return "right";
    case DIR_LEFT:
      return "left";
  }
  ASSERT(false, "invalid direction: {}\n", i32(direction));
}

f32 rotation_degrees(Direction rotation) {
  switch (rotation) {
    case DIR_UP:
      return 0;
    case DIR_DOWN:
      return 180;
    case DIR_RIGHT:
      return 90;
    case DIR_LEFT:
      return 270;
  }
  ASSERT(false, "invalid rotation: {}\n", i32(rotation));
}

vec2 rotate_vec2(const vec2& v, Direction rotation) {
  switch (rotation) {
    case DIR_UP:
      return {v.x, v.y};
    case DIR_RIGHT:
      return {-v.y, v.x};
    case DIR_DOWN:
      return {-v.x, v.y};
    case DIR_LEFT:
      return {v.y, -v.x};
  }
  ASSERT(false, "invalid rotation: {}\n", i32(rotation));
}

Rectangle rect(const vec2& pos, const vec2& dims, Direction rotation) {
  vec2 origin = pos + vec2{0.5f, 0.5f};
  vec2 a      = rotate_vec2({-0.5f, -0.5f}, rotation);
  vec2 b      = rotate_vec2(dims - vec2{0.5f, 0.5f}, rotation);
  vec2 lo     = {std::min(a.x, b.x), std::min(a.y, b.y)};
  vec2 hi     = {std::max(a.x, b.x), std::max(a.y, b.y)};
  return rect_from_vec2x2(origin + lo, hi - lo);
}

// TODO: seed it always in the same way in debug builds?
std::mt19937 g_random_mt = random_generate();

vec2 grid_pos(const vec2& pos) {
  return {std::floor(pos.x / GRID_DIMS.x), std::floor(pos.y / GRID_DIMS.y)};
}

std::string get_time_string(u64 time) {
  return std::format("{:02}:{:02} DAY: {}", (time / 60) % 24, time % 60, (time / 60) / 24);
}
