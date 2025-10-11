#ifndef ASSETS_H
#define ASSETS_H

struct AssetManager {
  std::unordered_map<std::string, Texture> textures;
  std::unordered_map<std::string, Material> materials;
};

static AssetManager g_assets{};

#endif

