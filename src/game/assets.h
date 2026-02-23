#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>

#include "base/base.h"
#include "base/math.h"
#include "base/uuid.h"

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

struct TextureData
{
  Image image;
  TextureWrappingOption wrap_s;
  TextureWrappingOption wrap_t;
  TextureFilteringOption min_filter;
  TextureFilteringOption mag_filter;
};
struct TextureHandle : public UUID
{
};

struct Material
{
  vec3 diffuse_color{};
  vec3 specular_color{};
  f32 specular_exponent{};

  std::optional<TextureHandle> diffuse_map{};

  ShaderHandle shader{};
};
struct MaterialHandle : public UUID
{
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

struct MeshData
{
  std::vector<Vertex> vertices;
  std::vector<u32> indices;

  std::vector<Submesh> submeshes;
  RenderPrimitive primitive;
};
struct MeshHandle : public UUID
{
};

extern MeshHandle static_model_cube_wires;
extern MeshHandle static_model_ring;
extern MeshHandle static_model_line;
static constexpr usize STATIC_MODEL_COUNT = 3;

class AssetManager
{
public:
  MeshHandle load_obj(const std::filesystem::path& path);
  void clear();

  [[nodiscard]] inline constexpr const TextureData& get(TextureHandle handle) const
  {
    return m_textures.at(handle);
  }
  [[nodiscard]] inline constexpr const Material& get(MaterialHandle handle) const
  {
    return m_materials.at(handle);
  }
  [[nodiscard]] inline constexpr const MeshData& get(MeshHandle handle) const
  {
    return m_meshes.at(handle);
  }

  inline constexpr TextureHandle set(TextureData&& texture)
  {
    TextureHandle handle{};
    m_textures.insert_or_assign(handle, std::move(texture));
    return handle;
  }
  inline constexpr MaterialHandle set(Material&& material)
  {
    MaterialHandle handle{};
    m_materials.insert_or_assign(handle, std::move(material));
    return handle;
  }
  inline constexpr MeshHandle set(MeshData&& mesh)
  {
    MeshHandle handle{};
    m_meshes.insert_or_assign(handle, std::move(mesh));
    return handle;
  }

  [[nodiscard]] inline constexpr bool contains(TextureHandle handle) const
  {
    return m_textures.contains(handle);
  }
  [[nodiscard]] inline constexpr bool contains(MaterialHandle handle) const
  {
    return m_materials.contains(handle);
  }
  [[nodiscard]] inline constexpr bool contains(MeshHandle handle) const
  {
    return m_meshes.contains(handle);
  }

private:
  TextureHandle obj_get_texture_by_path(const std::filesystem::path& path);
  void load_mtl_file(const std::filesystem::path& path);

private:
  std::unordered_map<TextureHandle, TextureData> m_textures;
  std::unordered_map<MaterialHandle, Material> m_materials;
  std::unordered_map<MeshHandle, MeshData> m_meshes;

  std::unordered_map<std::string, TextureHandle> m_texture_handles;
  std::unordered_map<std::string, MaterialHandle> m_material_handles;
};
