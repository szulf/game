#pragma once

#include "badtl/array.hpp"
#include "badtl/types.hpp"

namespace core {

enum class ShaderType {
  Vertex,
  Fragment,
};

enum class Shader {
  // NOTE(szulf): this should be last
  Default,
};

enum class ShaderError {
  InvalidVertex,
  InvalidFragment,
  CouldntLink,
};

struct ShaderMap {

  static ShaderMap make();

  btl::u32 operator[](Shader shader) const noexcept {
    return map[static_cast<btl::usize>(shader)];
  }
  static ShaderMap& instance() noexcept {
    static auto sm = ShaderMap::make();
    return sm;
  }

  btl::Array<btl::u32, static_cast<btl::usize>(Shader::Default) + 1> map;
};

}
