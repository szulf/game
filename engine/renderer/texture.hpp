#pragma once

#include <cstdint>

#include "image.hpp"

namespace core {

struct Texture final {
#ifdef GAME_OPENGL
  struct BackendData final {
    std::uint32_t id{};
  };
#else
#  error Unknown rendering backend
#endif

  // TODO(szulf): create placeholder texture
  constexpr Texture() {}
  Texture(const Image& img) noexcept;
  ~Texture();
  Texture(const Texture& other) = delete;
  Texture& operator=(const Texture& other) = delete;
  Texture(Texture&& other);
  Texture& operator=(Texture&& other);

  BackendData backend_data{};
};

}
