#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>

#include "base/base.h"
#include "base/math.h"

#include "vertex.h"
#include "image.h"

enum class ShaderHandle
{
  DEFAULT,
  LIGHTING,
  SHADOW_DEPTH,

  COUNT,
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

using TextureHandle = usize;
struct TextureData
{
  Image image;
  TextureWrappingOption wrap_s;
  TextureWrappingOption wrap_t;
  TextureFilteringOption min_filter;
  TextureFilteringOption mag_filter;
};

using MaterialHandle = usize;
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

using MeshHandle = usize;
struct MeshData
{
  std::vector<Vertex> vertices;
  std::vector<u32> indices;

  std::vector<Submesh> submeshes;
  RenderPrimitive primitive;
};

template <typename Handle, typename T>
class AssetType
{
public:
  Handle set(T&& t)
  {
    m_data.emplace_back(std::move(t));
    return m_data.size() - 1;
  }

  const T& get(Handle handle) const
  {
    return m_data[(usize) handle];
  }

  void clear()
  {
    m_data.clear();
  }

  [[nodiscard]] inline constexpr usize size() const
  {
    return m_data.size();
  }

private:
  std::vector<T> m_data{};
};

class AssetManager
{
public:
  MeshHandle load_obj(const std::filesystem::path& path);
  void clear();

  inline constexpr static AssetManager& instance()
  {
    static AssetManager a{};
    return a;
  }

private:
  AssetManager() {}

  TextureHandle obj_get_texture_by_path(const std::filesystem::path& path);
  void load_mtl_file(const std::filesystem::path& path);

public:
  AssetType<TextureHandle, TextureData> textures;
  AssetType<MaterialHandle, Material> materials;
  AssetType<MeshHandle, MeshData> meshes;

private:
  std::unordered_map<std::string, TextureHandle> m_texture_handles;
  std::unordered_map<std::string, MaterialHandle> m_material_handles;
};
