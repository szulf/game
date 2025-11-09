#ifndef ASSETS_H
#define ASSETS_H

namespace assets
{

static std::unordered_map<std::string, Texture> g_textures;
static std::unordered_map<std::string, Material> g_materials;

static bool texture_exists(const std::string& name);
static void texture_set(const std::string& name, const Texture& texture);
static Texture* texture_get(const std::string& name);

static bool material_exists(const std::string& name);
static void material_set(const std::string& name, const Material& material);
static Material* material_get(const std::string& name);

}

#endif
