#pragma once

#include <cstdint>
#include <array>

namespace core
{

enum class Shader : std::uint8_t
{
  // NOTE(szulf): this should be last
  Default,
};

class ShaderMap
{
public:
  ShaderMap();

  inline auto operator[](Shader shader) const -> std::uint32_t
  {
    return m_map[static_cast<std::size_t>(shader)];
  }
  inline static auto instance() -> ShaderMap&
  {
    static ShaderMap map{};
    return map;
  }

private:
  std::array<std::uint32_t, static_cast<std::size_t>(Shader::Default) + 1> m_map{};
};

}
