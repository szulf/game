#include "assets.h"

static void
setup_assets(Arena* arena, Error* err)
{
  Error error = ERROR_SUCCESS;

  g_texture_names = array_make<String>(10, arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
  g_textures  = array_make<Texture>(10, arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);

  g_material_names = array_make<String>(10, arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
  g_materials = array_make<Material>(10, arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
}

static b32
assets_texture_exists(String* name)
{
  ASSERT(g_texture_names.len == g_textures.len, "the name and texture array have to be in sync");
  for (usize i = 0; i < g_texture_names.len; ++i)
  {
    if (mem_compare(name->data, g_texture_names[i].data,
                    umin(name->len, g_texture_names[i].len)))
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
  array_push(&g_texture_names, *name);
  array_push(&g_textures, *texture);
}

static Texture*
assets_texture_get(String* name, Error* err)
{
  ASSERT(g_texture_names.len == g_textures.len, "the name and texture array have to be in sync");
  for (usize i = 0; i < g_texture_names.len; ++i)
  {
    if (mem_compare(name->data, g_texture_names[i].data,
                    umin(name->len, g_texture_names[i].len)))
    {
      return &g_textures[i];
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
    if (mem_compare(name->data, g_material_names[i].data,
                    umin(name->len, g_material_names[i].len)))
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
  array_push(&g_material_names, *name);
  array_push(&g_materials, *material);
}

static Material*
assets_material_get(String* name, Error* err)
{
  ASSERT(g_material_names.len == g_materials.len, "the name and material array have to be in sync");
  for (usize i = 0; i < g_material_names.len; ++i)
  {
    if (mem_compare(name->data, g_material_names[i].data,
                    umin(name->len, g_material_names[i].len)))
    {
      return &g_materials[i];
    }
  }
  *err = ERROR_NOT_FOUND;
  return 0;
}
