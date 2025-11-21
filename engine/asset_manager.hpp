#pragma once

#include "engine/renderer/texture.hpp"
#include "engine/renderer/material.hpp"
#include "badtl/map.hpp"
#include "badtl/string.hpp"

namespace core {

struct AssetManager {
  static AssetManager make(btl::Allocator& allocator);

  // TODO(szulf): this is really fuckin weird
  static AssetManager* instance;
  btl::Map<btl::String, Texture> textures;
  btl::Map<btl::String, Material> materials;
};

}
