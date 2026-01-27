#ifndef ASSETS_H
#define ASSETS_H

#include "base/base.h"
#include "base/math.h"
#include "base/vertex.h"
#include "base/array.h"
#include "base/string.h"

#include "image.h"

enum class ShaderHandle
{
  DEFAULT,
  LIGHTING,
  SHADOW_DEPTH,
};

enum class TextureWrappingOption
{
  REPEAT,
  MIRRORED_REPEAT,
  CLAMP_TO_EDGE,
  CLAMP_TO_BORDER,
};

enum class TextureFilteringOption
{
  LINEAR,
  NEAREST,
};

typedef usize TextureHandle;
struct TextureData
{
  Image image;
  TextureWrappingOption wrap_s;
  TextureWrappingOption wrap_t;
  TextureFilteringOption min_filter;
  TextureFilteringOption mag_filter;
};

typedef usize MaterialHandle;
struct Material
{
  vec3 diffuse_color;
  vec3 specular_color;
  f32 specular_exponent;

  TextureHandle diffuse_map;

  ShaderHandle shader;
};

struct Submesh
{
  usize index_offset;
  usize index_count;
  MaterialHandle material;
};

enum class RenderPrimitive
{
  TRIANGLES,
  LINE_STRIP,
};

enum StaticModel
{
  StaticModel_CUBE_WIRES,
  StaticModel_RING,
  StaticModel_LINE,

  StaticModel_COUNT,
};

typedef usize MeshHandle;
struct MeshData
{
  Array<Vertex> vertices;
  Array<u32> indices;

  Array<Submesh> submeshes;
  RenderPrimitive primitive;
};

template <typename Handle, typename T, usize AMOUNT>
struct AssetType
{
  Handle set(const T& t)
  {
    data[size++] = t;
    return size - 1;
  }

  const T& get(Handle handle) const
  {
    return data[(usize) handle];
  }

  void destroy_all()
  {
    size = 0;
    mem_set(data, 0, MAX * sizeof(T));
  }

  usize size;

private:
  static constexpr usize MAX = AMOUNT;
  T data[MAX];
};

struct Assets
{
  static Assets make(Allocator& allocator);

  MeshHandle load_obj(const char* path, Allocator& allocator, Error& out_error);

  void destroy_all();

  AssetType<TextureHandle, TextureData, 100> textures;
  AssetType<MaterialHandle, Material, 100> materials;
  AssetType<MeshHandle, MeshData, 100> meshes;

  Map<String, TextureHandle> texture_handles;
  Map<String, MaterialHandle> material_handles;
};

#endif
