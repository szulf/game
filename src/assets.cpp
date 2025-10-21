#include "assets.h"

namespace assets {

static void
setup(mem::Arena& arena, Error* err)
{
  Error error = Error::SUCCESS;

  g_texture_names = Array<String>::make(10, arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error,);
  g_textures  = Array<Texture>::make(10, arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error,);

  g_material_names = Array<String>::make(10, arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error,);
  g_materials = Array<Material>::make(10, arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error,);
}

static b32
texture_exists(const String& name)
{
  ASSERT(g_texture_names.len == g_textures.len, "the name and texture array have to be in sync");
  for (const auto& texture_name : g_texture_names)
  {
    if (mem::compare(name.data, texture_name.data,
                     umin(name.len, texture_name.len)))
    {
      return true;
    }
  }
  return false;
}

static void
texture_set(const String& name, const Texture& texture)
{
  ASSERT(g_texture_names.len == g_textures.len, "the name and texture array have to be in sync");
  g_texture_names.push(name);
  g_textures.push(texture);
}

static Texture*
texture_get(const String& name, Error* err)
{
  ASSERT(g_texture_names.len == g_textures.len, "the name and texture array have to be in sync");
  for (usize i = 0; i < g_texture_names.len; ++i)
  {
    if (mem::compare(name.data, g_texture_names[i].data,
                     umin(name.len, g_texture_names[i].len)))
    {
      return &g_textures[i];
    }
  }
  *err = Error::NOT_FOUND;
  return 0;
}

static b32
material_exists(const String& name)
{
  ASSERT(g_material_names.len == g_materials.len, "the name and material array have to be in sync");
  for (const auto& material_name : g_material_names)
  {
    if (mem::compare(name.data, material_name.data,
                     umin(name.len, material_name.len)))
    {
      return true;
    }
  }
  return false;
}

static void
material_set(const String& name, const Material& material)
{
  ASSERT(g_material_names.len == g_materials.len, "the name and material array have to be in sync");
  g_material_names.push(name);
  g_materials.push(material);
}

static Material*
material_get(const String& name, Error* err)
{
  ASSERT(g_material_names.len == g_materials.len, "the name and material array have to be in sync");
  for (usize i = 0; i < g_material_names.len; ++i)
  {
    if (mem::compare(name.data, g_material_names[i].data,
                     umin(name.len, g_material_names[i].len)))
    {
      return &g_materials[i];
    }
  }
  *err = Error::NOT_FOUND;
  return 0;
}

}

