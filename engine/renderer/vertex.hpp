#pragma once

#include "badtl/types.hpp"
#include "badtl/vec3.hpp"
#include "badtl/vec2.hpp"

namespace core {

struct Vertex {
  btl::Vec3 position;
  btl::Vec3 normal;
  btl::Vec2 uv;

  bool operator==(const Vertex& other) const noexcept {
    return position == other.position && normal == other.normal && uv == other.uv;
  }
};

}

btl::usize hash(const core::Vertex& v);
