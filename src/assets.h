#ifndef ASSETS_H
#define ASSETS_H

// TODO(szulf): if it turns out i will store a bunch of textures/materials switch this to a hashmap
static Array<String> g_texture_names;
static Array<Texture> g_textures;

static Array<String> g_material_names;
static Array<Material> g_materials;

static void setup_assets(Arena* arena, Error* err);

static b32 assets_texture_exists(String* name);
static void assets_texture_set(String* name, Texture* texture);
static Texture* assets_texture_get(String* name, Error* err);

static b32 assets_material_exists(String* name);
static void assets_material_set(String* name, Material* material);
static Material* assets_material_get(String* name, Error* err);

#endif
