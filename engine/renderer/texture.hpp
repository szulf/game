#pragma once

#include "engine/image.hpp"

namespace core {

struct Texture final {
#ifdef GAME_OPENGL
  struct BackendData final {
    btl::u32 id;
  };
#else
#  error Unknown rendering backend
#endif

  // TODO(szulf): create placeholder texture
  static Texture make();
  // TODO(szulf): find a way to free gpu memory
  static Texture make(const Image& img);

  BackendData backend_data;
};

}
