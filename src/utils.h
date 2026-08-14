#pragma once

#include "raylib.h"

#include "core.h"
#include "math.h"

// NOTE: raylib doesnt have this
bool operator==(const Color& a, const Color& b);

constexpr Rectangle rect_from_vec2x2(const vec2& pos, const vec2& dims) {
  return {
    .x      = pos.x,
    .y      = pos.y,
    .width  = dims.x,
    .height = dims.y,
  };
}

vec2 pos_from_rect(const Rectangle& rect);
vec2 dims_from_rect(const Rectangle& rect);
vec2 dims_from_texture(const Texture2D& texture);

enum Direction {
  DIR_UP,
  DIR_RIGHT,
  DIR_DOWN,
  DIR_LEFT,
  DIR_COUNT,
};

Direction opposite_direction(Direction direction);
vec2 direction_to_vec2(Direction direction);
std::string_view direction_to_string(Direction direction);
f32 rotation_degrees(Direction rotation);

// TODO: should this really be here?
static constexpr vec2 GRID_DIMS = {32, 32};
static constexpr i32 TPS        = 60;
static constexpr f32 DT         = 1.0f / TPS;

inline std::mt19937 random_generate() {
  std::random_device rd{};
  std::seed_seq ss{rd()};
  return std::mt19937{ss};
}

extern std::mt19937 g_random_mt;

template <typename T>
inline T random_get(T min, T max) {
  if constexpr (is_any_of<T, u8, u16, u32, u64, i8, i16, i32, i64>) {
    return std::uniform_int_distribution<T>{min, max}(g_random_mt);
  } else if constexpr (is_any_of<T, f32, f64>) {
    return std::uniform_real_distribution<>{min, max}(g_random_mt);
  } else {
    static_assert(false, "invalid random number type");
  }
}

vec2 grid_pos(const vec2& pos);
