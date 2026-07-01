enum TextureType {
  TEXTURE_PLAYER,
  TEXTURE_BLOCK,
  TEXTURE_BLOCK_ITEM,
  TEXTURE_STORAGE,
  TEXTURE_STORAGE_ITEM,
  TEXTURE_CONVEYOR,
  TEXTURE_CONVEYOR_ITEM,
  TEXTURE_WORLD_TUNNEL,

  TEXTURE_COUNT,
};

static constexpr std::array<std::string_view, TEXTURE_COUNT> TEXTURE_PATHS = []() {
  std::array<std::string_view, TEXTURE_COUNT> paths{};
  paths[TEXTURE_PLAYER]        = "assets/player.png";
  paths[TEXTURE_BLOCK]         = "assets/block.png";
  paths[TEXTURE_BLOCK_ITEM]    = "assets/block_item.png";
  paths[TEXTURE_STORAGE]       = "assets/storage.png";
  paths[TEXTURE_STORAGE_ITEM]  = "assets/storage_item.png";
  paths[TEXTURE_CONVEYOR]      = "assets/conveyor.png";
  paths[TEXTURE_CONVEYOR_ITEM] = "assets/conveyor_item.png";
  paths[TEXTURE_WORLD_TUNNEL]  = "assets/world_tunnel.png";
  return paths;
}();

struct AssetManager {
  std::array<Texture2D, TEXTURE_COUNT> textures{};
};

void load_textures(AssetManager& assets) {
  for (u32 i = 0; i < TEXTURE_COUNT; ++i) {
    assets.textures[i] = LoadTexture(TEXTURE_PATHS[i].data());
  }
}
