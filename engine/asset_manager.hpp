#pragma once

#include <string>
#include <unordered_map>

#include "renderer/texture.hpp"
#include "renderer/material.hpp"

namespace core {

struct AssetManager {
  static constexpr AssetManager& instance() {
    static AssetManager am{};
    return am;
  }

  std::unordered_map<std::string, Texture> textures{};
  std::unordered_map<std::string, Material> materials{};
};

}
