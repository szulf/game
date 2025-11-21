#include "engine/asset_manager.hpp"

namespace core {

AssetManager* AssetManager::instance = nullptr;

AssetManager AssetManager::make(btl::Allocator& allocator) {
  AssetManager out = {};
  // TODO(szulf): monitor the usage of these and adjust as needed
  out.textures = btl::Map<btl::String, Texture>::make(100, allocator);
  out.materials = btl::Map<btl::String, Material>::make(100, allocator);
  return out;
}

}
