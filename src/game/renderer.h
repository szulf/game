#ifndef RENDERER_H
#define RENDERER_H

#include <unordered_map>
#include <vector>

#include "base/base.h"

#include "assets.h"
#include "camera.h"

enum class ShaderType
{
  VERTEX,
  FRAGMENT,
  GEOMETRY,
};

class Shader
{
public:
  Shader(ShaderHandle handle);
  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;
  Shader(Shader&& other);
  Shader& operator=(Shader&& other);
  ~Shader();

  [[nodiscard]] inline constexpr u32 handle() const noexcept
  {
    return m_id;
  }

private:
  u32 m_id{};
};

class TextureGPU
{
public:
  TextureGPU(TextureHandle handle, AssetManager& asset_manager);
  TextureGPU(const TextureGPU&) = delete;
  TextureGPU& operator=(const TextureGPU&) = delete;
  TextureGPU(TextureGPU&& other);
  TextureGPU& operator=(TextureGPU&& other);
  ~TextureGPU();

  [[nodiscard]] inline constexpr u32 handle() const noexcept
  {
    return m_id;
  }

private:
  u32 m_id{};
};

// TODO: i dont like this
struct RenderData
{
  u32 camera_ubo{};
  u32 lights_ubo{};

  static constexpr uvec2 SHADOW_CUBEMAP_DIMENSIONS = {1024, 1024};
  // TODO: this should go through the TextureGPU class, but it doesnt handle cubemaps yet
  u32 shadow_cubemap{};
  u32 shadow_framebuffer_id{};

  u32 instance_data_buffer{};
};

class MeshGPU
{
public:
  MeshGPU(MeshHandle handle, RenderData& render_data, AssetManager& asset_manager);
  MeshGPU(const MeshGPU&) = delete;
  MeshGPU& operator=(const MeshGPU&) = delete;
  MeshGPU(MeshGPU&& other);
  MeshGPU& operator=(MeshGPU&& other);
  ~MeshGPU();

  [[nodiscard]] inline constexpr u32 handle() const noexcept
  {
    return m_vao;
  }

private:
  u32 m_vao{};
  u32 m_vbo{};
  u32 m_ebo{};
};

class AssetGPUManager
{
public:
  AssetGPUManager(RenderData& render_data, AssetManager& asset_manager)
    : m_asset_manager{asset_manager}, m_render_data{render_data}
  {
  }

  [[nodiscard]] inline constexpr const Shader& get(ShaderHandle handle) const
  {
    return m_shaders.at(handle);
  }
  [[nodiscard]] inline constexpr const TextureGPU& get(TextureHandle handle) const
  {
    return m_textures.at(handle);
  }
  [[nodiscard]] inline constexpr const MeshGPU& get(MeshHandle handle) const
  {
    return m_meshes.at(handle);
  }

  inline constexpr void create(ShaderHandle handle)
  {
    m_shaders.insert_or_assign(handle, Shader{handle});
  }
  inline constexpr void create(TextureHandle handle)
  {
    m_textures.insert_or_assign(handle, TextureGPU{handle, m_asset_manager});
  }
  inline constexpr void create(MeshHandle handle)
  {
    m_meshes.insert_or_assign(handle, MeshGPU{handle, m_render_data, m_asset_manager});
  }

  [[nodiscard]] inline constexpr bool contains(ShaderHandle handle) const
  {
    return m_shaders.contains(handle);
  }
  [[nodiscard]] inline constexpr bool contains(TextureHandle handle) const
  {
    return m_textures.contains(handle);
  }
  [[nodiscard]] inline constexpr bool contains(MeshHandle handle) const
  {
    return m_meshes.contains(handle);
  }

private:
  AssetManager& m_asset_manager;
  RenderData& m_render_data;

  std::unordered_map<ShaderHandle, Shader> m_shaders;
  std::unordered_map<TextureHandle, TextureGPU> m_textures;
  std::unordered_map<MeshHandle, MeshGPU> m_meshes;
};

struct InstanceData
{
  static constexpr usize MAX = 10000;
  mat4 transform;
  vec3 tint;
};

struct RenderCmd
{
  MaterialHandle material;
  MeshHandle mesh;
  usize submesh_idx;
  InstanceData instance_data;
};

struct Light
{
  vec3 pos;
  vec3 color;

  static constexpr f32 CONSTANT = 1.0f;
  static constexpr f32 LINEAR = 0.22f;
  static constexpr f32 QUADRATIC = 0.2f;
};

enum class RenderPassType
{
  FORWARD,
  POINT_SHADOW_MAP,
};

class RenderPass
{
public:
  // NOTE: 3d
  void draw_mesh(
    MeshHandle handle,
    const vec3& pos,
    f32 rotation,
    const vec3& tint = {1.0f, 1.0f, 1.0f}
  );
  void draw_cube_wires(const vec3& pos, const vec3& size, const vec3& color);
  void draw_ring(const vec3& pos, f32 radius, const vec3& color);
  void draw_line(const vec3& pos, f32 length, f32 rotation, const vec3& color);

  // NOTE: supports rendering only a single point light since that is what i need for the game,
  // would not be too hard to implement more lights (will implement directional light later)
  void set_light(const vec3& pos, const vec3& color);
  // TODO: i feel like this method should take more arguments,
  // probably will fix itself when i will implement the directional light shadow map
  void use_shadow_map(const Camera& shadow_map_camera);

  void finish();

private:
  inline constexpr RenderPass(
    RenderPassType type,
    const Camera& camera,
    const vec3& ambient_color,
    RenderData& render_data,
    AssetManager& asset_manager,
    AssetGPUManager& asset_gpu_manager
  )
    : m_asset_manager{asset_manager}, m_asset_gpu_manager{asset_gpu_manager},
      m_render_data{render_data}, m_type{type}, m_camera{camera}, m_ambient_color{ambient_color}
  {
  }

private:
  AssetManager& m_asset_manager;
  AssetGPUManager& m_asset_gpu_manager;
  RenderData& m_render_data;

  // TODO: i dont like having a pass 'type'
  RenderPassType m_type{};
  const Camera& m_camera;
  std::vector<RenderCmd> m_cmds{};
  Light m_light{};
  vec3 m_ambient_color{};
  const Camera* m_shadow_map_camera{};

  friend class Renderer;
};

class Renderer
{
public:
  Renderer(AssetManager& asset_manager);

  RenderPass
  begin_pass(RenderPassType type, const Camera& camera, const vec3& ambient_color = {1, 1, 1});

private:
  AssetManager& m_asset_manager;
  RenderData m_render_data{};
  AssetGPUManager m_asset_gpu_manager{m_render_data, m_asset_manager};
};

enum UBO_Index
{
  UBO_INDEX_CAMERA = 0,
  UBO_INDEX_LIGHTS,
};

struct STD140Camera
{
  mat4 proj_view;
  vec3 view_pos;
  float far_plane;
};

struct STD140Light
{
  vec4 pos;
  vec4 color;

  f32 constant;
  f32 linear;
  f32 quadratic;
  f32 _pad;
};

#endif
