#pragma once

#include "math.hpp"

namespace core
{

struct Vertex
{
  math::vec3 position{};
  math::vec3 normal{};
  math::vec2 uv{};
};

}
