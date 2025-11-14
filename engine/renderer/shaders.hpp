#pragma once

#include <cstdint>
#include <array>

namespace core {

enum class ShaderType : std::uint8_t {
  Vertex,
  Fragment,
};

enum class Shader : std::uint8_t {
  // NOTE(szulf): this should be last
  Default,
};

struct ShaderMap final {
  ShaderMap();

  constexpr std::uint32_t operator[](Shader shader) const noexcept {
    return m_map[static_cast<std::size_t>(shader)];
  }
  static constexpr ShaderMap& instance() noexcept {
    static ShaderMap map{};
    return map;
  }

  std::array<std::uint32_t, static_cast<std::size_t>(Shader::Default) + 1> m_map{};
};

}
