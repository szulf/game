#ifndef RENDERER_H
#define RENDERER_H

#include "base/base.h"
#include "base/map.h"

#include "assets.h"
#include "camera.h"

enum class ShaderType
{
  VERTEX,
  FRAGMENT,
  GEOMETRY,
};

struct Shader
{
  u32 id;
};

struct TextureGPU
{
  u32 id;
};

struct MeshGPU
{
  u32 vao;
  u32 vbo;
  u32 ebo;
};

struct RenderData
{
  u32 camera_ubo;
  u32 lights_ubo;

  static constexpr usize SHADOW_CUBEMAP_WIDTH = 1024;
  static constexpr usize SHADOW_CUBEMAP_HEIGHT = 1024;
  TextureGPU shadow_cubemap;
  u32 shadow_framebuffer_id;

  u32 instance_data_buffer;
};

template <typename Handle, typename T>
struct AssetTypeGPU
{
  // NOTE: creates the gpu resource, specified for each asset type (shaders are an exception)
  void create(Handle handle);

  const T& get(Handle handle) const
  {
    return *data[handle];
  }

  bool contains(Handle handle) const
  {
    return data.contains(handle);
  }

  void destroy(Handle handle);
  void destroy_all();

  Map<Handle, T> data;
  Assets* assets;
};

struct AssetsGPU
{
  static AssetsGPU make(Assets& assets, Allocator& allocator);
  void destroy_all();

  AssetTypeGPU<ShaderHandle, Shader> shaders;
  AssetTypeGPU<TextureHandle, TextureGPU> textures;
  AssetTypeGPU<MeshHandle, MeshGPU> meshes;
};

struct InstanceData
{
  static constexpr usize MAX = 10000;
  mat4 transform;
  vec3 tint;
};

struct RenderItem
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

struct RenderPass
{
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

  RenderPassType type;

  Assets* assets;
  AssetsGPU* assets_gpu;
  const Camera* camera;

  Array<RenderItem> items;
  Light light;
  vec3 ambient_color;

  const Camera* shadow_map_camera;
};

struct Renderer
{
  static Renderer make(Assets& assets, Allocator& allocator);

  RenderPass begin_pass(
    RenderPassType type,
    const Camera& camera,
    Allocator& allocator,
    const vec3& ambient_color = {1.0f, 1.0f, 1.0f}
  );

  Assets* assets;
  AssetsGPU assets_gpu;
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
