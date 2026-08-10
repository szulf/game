#include "utils.h"

bool operator==(const Color& a, const Color& b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
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

std::string_view direction_to_string(Direction direction) {
  switch (direction) {
    case Direction::UP:
      return "up";
    case Direction::DOWN:
      return "down";
    case Direction::RIGHT:
      return "right";
    case Direction::LEFT:
      return "left";
    case Direction::COUNT:
      break;
  }
  ASSERT(false, "invalid direction: %d\n", i32(direction));
}

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

// TODO: seed it always in the same way in debug builds?
std::mt19937 g_random_mt = random_generate();

vec2 grid_pos(const vec2& pos) {
  return {std::floor(pos.x / GRID_DIMS.x), std::floor(pos.y / GRID_DIMS.y)};
}
