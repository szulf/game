#ifndef ASSETS_H
#define ASSETS_H

namespace assets {

// TODO(szulf): if it turns out i will store a bunch of textures/materials switch this to a hashmap
static Array<String> g_texture_names;
static Array<Texture> g_textures;

static Array<String> g_material_names;
static Array<Material> g_materials;

static void setup(mem::Arena& arena, Error* err);

static b32 texture_exists(const String& name);
static void texture_set(const String& name, const Texture& texture);
static Texture* texture_get(const String& name, Error* err);

static b32 material_exists(const String& name);
static void material_set(const String& name, const Material& material);
static Material* material_get(const String& name, Error* err);

}

#endif
