#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

// TODO(szulf): i should probably implement a string hashmap for this

typedef struct Assets
{
  StringArray texture_names;
  TextureArray textures;

  StringArray material_names;
  MaterialArray materials;
} Assets;

static Assets g_assets = {0};

static void assets_setup(Arena* arena, Error* err);

static void assets_texture_new(String* texture_name, Texture* texture);

static void assets_material_new(String* material_name, Material* material);
static Material* assets_material_get(String* material_name, Error* err);
static b32 assets_material_exists(String* material_name);

#endif
