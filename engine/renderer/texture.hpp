#pragma once

#include <cstdint>

namespace core
{

class Texture
{
private:
#ifdef GAME_OPENGL
  struct BackendData
  {
    std::uint32_t id{};
  };
#else
#  error Unknown rendering backend
#endif

public:
  Texture();

  inline auto backendData() const noexcept -> const BackendData&
  {
    return m_backend_data;
  }

private:
  BackendData m_backend_data{};
};

}
