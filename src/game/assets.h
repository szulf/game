#ifndef ASSETS_H
#define ASSETS_H

enum class ShaderHandle
{
  DEFAULT,
  LIGHTING,
  SHADOW_DEPTH,
};

typedef usize TextureHandle;
struct TextureData
{
  Image image;
  // TODO: options
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

template <typename Handle, typename T>
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

  // TODO: should not destroy static models
  void destroy_all()
  {
    size = 0;
    mem_set(data, 0, MAX * sizeof(T));
  }

  usize size;

private:
  // TODO: take from template?
  static constexpr usize MAX = 100;
  T data[MAX];
};

struct Assets
{
  static Assets make(Allocator& allocator);

  MeshHandle load_obj(const char* path, Allocator& allocator, Error& out_error);

  void destroy_all();

  AssetType<TextureHandle, TextureData> textures;
  AssetType<MaterialHandle, Material> materials;
  AssetType<MeshHandle, MeshData> meshes;

  Map<String, TextureHandle> texture_handles;
  Map<String, MaterialHandle> material_handles;
};

#endif
