#include "assets.h"

namespace assets
{

static bool
texture_exists(const std::string& name)
{
  return g_textures.contains(name);
}

static void
texture_set(const std::string& name, const Texture& texture)
{
  g_textures.insert_or_assign(name, texture);
}

static Texture*
texture_get(const std::string& name)
{
  auto it = g_textures.find(name);
  if (it == g_textures.end())
  {
    return nullptr;
  }
  return &it->second;
}

static bool
material_exists(const std::string& name)
{
  return g_materials.contains(name);
}

static void
material_set(const std::string& name, const Material& material)
{
  g_materials.insert_or_assign(name, material);
}

static Material*
material_get(const std::string& name)
{
  auto it = g_materials.find(name);
  if (it == g_materials.end())
  {
    return nullptr;
  }
  return &it->second;
}

}
