#include "assets.h"

static void
assets_setup(Arena* arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  // TODO(szulf): what should the sizes be?
  ARRAY_INIT(&g_assets.texture_names, arena, 10, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
  ARRAY_INIT(&g_assets.textures, arena, 10, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);

  ARRAY_INIT(&g_assets.material_names, arena, 10, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
  ARRAY_INIT(&g_assets.materials, arena, 10, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
}

static void
assets_texture_new(String* texture_name, Texture* texture)
{
  ASSERT(g_assets.texture_names.len == g_assets.textures.len,
         "string and textures arrays have to have the same length");
  ARRAY_PUSH(&g_assets.texture_names, *texture_name);
  ARRAY_PUSH(&g_assets.textures, *texture);
}

static void
assets_material_new(String* material_name, Material* material)
{
  ASSERT(g_assets.material_names.len == g_assets.materials.len,
         "string and materials arrays have to have the same length");
  ARRAY_PUSH(&g_assets.material_names, *material_name);
  ARRAY_PUSH(&g_assets.materials, *material);
}

static Material*
assets_material_get(String* material_name, Error* err)
{
  ASSERT(g_assets.material_names.len == g_assets.materials.len,
         "string and materials arrays have to have the same length");
  usize idx = (usize) -1;
  for (usize i = 0; i < g_assets.material_names.len; ++i)
  {
    if (mem_compare(g_assets.material_names.items[i].data, material_name->data,
                    umin(g_assets.material_names.items[i].len, material_name->len)))
    {
      idx = i;
      break;
    }
  }
  ERROR_ASSERT(idx != (usize) -1, *err, ERROR_NOT_FOUND, 0);
  return &g_assets.materials.items[idx];
}

static b32
assets_material_exists(String* material_name)
{
  for (usize i = 0; i < g_assets.material_names.len; ++i)
  {
    if (mem_compare(g_assets.material_names.items[i].data, material_name->data,
                    umin(g_assets.material_names.items[i].len, material_name->len)))
    {
      return true;
    }
  }

  return false;
}
