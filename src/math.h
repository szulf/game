#pragma once

#include <cmath>

#include "core.h"

struct vec2 {
  f32 x{};
  f32 y{};
};

constexpr vec2 operator+(const vec2& a, const vec2& b) {
  return {a.x + b.x, a.y + b.y};
}

constexpr vec2 operator-(const vec2& a, const vec2& b) {
  return {a.x - b.x, a.y - b.y};
}

constexpr vec2 operator*(const vec2& a, f32 scalar) {
  return {a.x * scalar, a.y * scalar};
}

constexpr vec2 operator*(const vec2& a, const vec2& b) {
  return {a.x * b.x, a.y * b.y};
}

constexpr vec2 operator/(const vec2& a, i32 b) {
  return {a.x / b, a.y / b};
}

constexpr vec2 operator/(const vec2& a, const vec2& b) {
  return {a.x / b.x, a.y / b.y};
}

constexpr vec2& operator+=(vec2& a, const vec2& b) {
  a.x += b.x;
  a.y += b.y;
  return a;
}

constexpr bool operator==(const vec2& a, const vec2& b) {
  return a.x == b.x && a.y == b.y;
}

constexpr f32 length(const vec2& v) {
  return std::hypot(v.x, v.y);
}

constexpr f32 length2(const vec2& v) {
  return (v.x * v.x) + (v.y * v.y);
}
