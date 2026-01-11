#ifndef RENDERER_H
#define RENDERER_H

namespace renderer
{

enum class RenderingAPI
{
  OPENGL,
};

enum class ShaderType
{
  VERTEX,
  FRAGMENT,
  GEOMETRY,
};

typedef u32 ShaderOpenGL;

struct Shader
{
  union Data
  {
    ShaderOpenGL opengl;
  };
  Data data;

  static Shader
  from_file(const char* vert_path, const char* frag_path, const char* geom_path, Error& out_error);
};

struct TextureOpenGL
{
  u32 id;
};

struct TextureGPU
{
  union Data
  {
    TextureOpenGL opengl;
  };
  Data data;

  static TextureGPU make(assets::TextureHandle handle);
};

struct MeshOpenGL
{
  u32 vao;
  u32 vbo;
  u32 ebo;
};

struct MeshGPU
{
  union Data
  {
    MeshOpenGL opengl;
  };
  Data data;

  static MeshGPU make(assets::MeshHandle handle);
};

struct ManagerGPU
{
  Map<assets::ShaderHandle, Shader> shaders;
  Map<assets::TextureHandle, TextureGPU> textures;
  Map<assets::MeshHandle, MeshGPU> meshes;

  static ManagerGPU make(Allocator& allocator);
  static ManagerGPU* instance;
};

void shader_set(assets::ShaderHandle handle, const Shader& shader);
Shader& shader_get(assets::ShaderHandle handle);
bool texture_gpu_exists(assets::TextureHandle handle);
void texture_gpu_set(assets::TextureHandle handle, const TextureGPU& texture);
TextureGPU& texture_gpu_get(assets::TextureHandle handle);
bool mesh_gpu_exists(assets::MeshHandle handle);
void mesh_gpu_set(assets::MeshHandle handle, const MeshGPU& mesh);
MeshGPU& mesh_gpu_get(assets::MeshHandle handle);

enum StaticModel
{
  STATIC_MODEL_BOUNDING_BOX = 1,
  STATIC_MODEL_RING,
  STATIC_MODEL_LINE,
};

void static_model_init(
  StaticModel static_model,
  assets::ShaderHandle shader,
  const Array<Vertex>& vertices,
  const Array<u32>& indices,
  RenderingPrimitive primitive,
  bool wireframe,
  vec3 color,
  Allocator& allocator
);

#define MAX_INSTANCES 10000
static u32 instancing_matrix_buffer;
struct InstanceData
{
  mat4 model;
  vec3 tint;
};

struct Item
{
  mat4 model;
  vec3 tint;
  assets::MeshHandle mesh;
  assets::MaterialHandle material;
};

#define MAX_LIGHTS 32
struct Light
{
  vec3 pos;
  vec3 color;
};

struct Pass
{
  Array<Item> items;
  Array<Light> lights;

  Camera camera;

  u32 transforms_count;
  mat4* transforms;

  bool override_shader;
  assets::ShaderHandle shader;

  u32 framebuffer_id;

  TextureGPU* shadow_map;
  float shadow_map_camera_far_plane;

  u32 width;
  u32 height;
};

Pass pass_make(Allocator& allocator);

void init(Allocator& allocator, Error& out_error);
void queue_items(Pass& pass, const Item& render_item);
void queue_items(Pass& pass, const Array<Item>& render_items);
void sort_items(Pass& pass);
void draw(const Pass& pass);

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

struct STD140Lights
{
  int light_count;
  vec3 _pad;
  STD140Light lights[MAX_LIGHTS];
};

}

#endif
