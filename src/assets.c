#include "assets.h"

static void
setup_assets(Arena* arena, Error* err)
{
  Error error = ERROR_SUCCESS;

  ARRAY_INIT(&g_texture_names, 10, arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
  ARRAY_INIT(&g_textures, 10, arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);

  ARRAY_INIT(&g_material_names, 10, arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
  ARRAY_INIT(&g_materials, 10, arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
}

static b32
assets_texture_exists(String* name)
{
  ASSERT(g_texture_names.len == g_textures.len, "the name and texture array have to be in sync");
  for (usize i = 0; i < g_texture_names.len; ++i)
  {
    if (mem_compare(name->data, g_texture_names.items[i].data,
                    umin(name->len, g_texture_names.items[i].len)))
    {
      return true;
    }
  }
  return false;
}

static void
assets_texture_set(String* name, Texture* texture)
{
  ASSERT(g_texture_names.len == g_textures.len, "the name and texture array have to be in sync");
  ARRAY_PUSH(&g_texture_names, *name);
  ARRAY_PUSH(&g_textures, *texture);
}

static Texture*
assets_texture_get(String* name, Error* err)
{
  ASSERT(g_texture_names.len == g_textures.len, "the name and texture array have to be in sync");
  for (usize i = 0; i < g_texture_names.len; ++i)
  {
    if (mem_compare(name->data, g_texture_names.items[i].data,
                    umin(name->len, g_texture_names.items[i].len)))
    {
      return &g_textures.items[i];
    }
  }
  *err = ERROR_NOT_FOUND;
  return 0;
}

static b32
assets_material_exists(String* name)
{
  ASSERT(g_material_names.len == g_materials.len, "the name and material array have to be in sync");
  for (usize i = 0; i < g_material_names.len; ++i)
  {
    if (mem_compare(name->data, g_material_names.items[i].data,
                    umin(name->len, g_material_names.items[i].len)))
    {
      return true;
    }
  }
  return false;
}

static void
assets_material_set(String* name, Material* material)
{
  ASSERT(g_material_names.len == g_materials.len, "the name and material array have to be in sync");
  ARRAY_PUSH(&g_material_names, *name);
  ARRAY_PUSH(&g_materials, *material);
}

static Material*
assets_material_get(String* name, Error* err)
{
  ASSERT(g_material_names.len == g_materials.len, "the name and material array have to be in sync");
  for (usize i = 0; i < g_material_names.len; ++i)
  {
    if (mem_compare(name->data, g_material_names.items[i].data,
                    umin(name->len, g_material_names.items[i].len)))
    {
      return &g_materials.items[i];
    }
  }
  *err = ERROR_NOT_FOUND;
  return 0;
}
