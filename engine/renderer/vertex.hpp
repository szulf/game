#pragma once

#include <format>
#include <functional>
#include <cstdint>

#include "math.hpp"

namespace core {

struct Vertex final {
  math::vec3 position{};
  math::vec3 normal{};
  math::vec2 uv{};

  constexpr bool operator==(const Vertex& other) const noexcept {
    return position == other.position && normal == other.normal && uv == other.uv;
  }
};

}

template <>
struct std::hash<core::Vertex> {
  static constexpr std::uint64_t FNV_OFFSET = 14695981039346656037UL;
  static constexpr std::uint64_t FNV_PRIME = 1099511628211UL;

  std::size_t operator()(const core::Vertex& v) const noexcept {
    std::size_t hash = FNV_OFFSET;
    for (std::size_t i{0}; i < sizeof(v); ++i) {
      hash ^= reinterpret_cast<const std::uint8_t*>(&v)[i];
      hash *= FNV_PRIME;
    }
    return hash;
  }
};

template <>
struct std::formatter<core::Vertex> {
  template <class T>
  constexpr T::iterator parse(T& ctx) {
    return ctx.begin();
  }

  template <class T>
  T::iterator format(const core::Vertex& v, T& ctx) const {
    return std::format_to(ctx.out(), "{} {} {}", v.position, v.normal, v.uv);
  }
};
