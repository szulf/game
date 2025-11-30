#ifndef GL_RENDERER_H
#define GL_RENDERER_H

// TODO(szulf): split this into multiple files

// NOTE(szulf): specialization needed for handles to work correctly
template <>
usize hash(const u64& value);

enum Shader
{
  SHADER_GREEN,
  SHADER_YELLOW,
  SHADER_DEFAULT,
};

enum ShaderType
{
  SHADER_TYPE_VERTEX,
  SHADER_TYPE_FRAGMENT,
};

enum ShaderError
{
  SHADER_ERROR_COMPILATION = GLOBAL_ERROR_COUNT,
  SHADER_ERROR_LINKING,
};

void shader_init(u32* shader_map);

static u32* shader_map_instance = nullptr;

struct Vertex
{
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
};

template <>
usize hash(const Vertex& vertex);
bool operator==(const Vertex& va, const Vertex& vb);

typedef usize TextureHandle;
typedef usize MaterialHandle;
typedef usize MeshHandle;
typedef usize ModelHandle;

struct Texture
{
  u32 id;
};

Texture texture_make(const Image& img);

struct Material
{
  TextureHandle texture;
  Shader shader;
};

struct Mesh
{
  u32 vao;
  u32 vbo;
  u32 ebo;
  Array<Vertex> vertices;
  Array<u32> indices;
  MaterialHandle material;
};

Mesh mesh_make(const Array<Vertex>& vertices, const Array<u32>& indices, MaterialHandle material);

struct Model
{
  Array<MeshHandle> meshes;
  Mat4 matrix;
};

static const Vec3 CAMERA_WORLD_UP = {0.0f, 1.0f, 0.0f};
#define CAMERA_SPEED 4.0f
#define CAMERA_SENSITIVITY 1.5f

struct Camera
{
  Vec3 pos;
  Vec3 front;
  Vec3 up;
  Vec3 right;

  f32 yaw;
  f32 pitch;

  f32 fov;
  f32 near_plane;
  f32 far_plane;

  u32 viewport_width;
  u32 viewport_height;
};

void camera_update_vectors(Camera& camera);

Mat4 camera_look_at(const Camera& camera);
Mat4 camera_projection(const Camera& camera);

// TODO(szulf): remove this after restructuring renderer
#include "assets.cpp"

enum Primitive
{
  PRIMITIVE_TRIANGLES,
  PRIMITIVE_LINE_STRIP,
};

struct DrawCall
{
  Mat4 model;
  Mat4 view;
  Mat4 projection;

  ModelHandle model_handle;

  bool wireframe;
  Primitive primitive;
};

void renderer_init();
void renderer_clear_screen();
void renderer_window_resize(u32 width, u32 height);
void renderer_queue_draw_call(const DrawCall& scene);
void renderer_draw();

#endif
