#ifndef OGL_RENDERER_H
#define OGL_RENDERER_H

typedef struct Vertex
{
  Vec3 pos;
} Vertex;

typedef struct VertexArray
{
  usize cap;
  usize len;
  Vertex* items;
} VertexArray;

typedef struct U32Array
{
  usize cap;
  usize len;
  u32* items;
} U32Array;

typedef struct Mesh
{
  VertexArray vertices;
  U32Array indices;

  u32 vao;
  u32 vbo;
  u32 ebo;
} Mesh;

static void mesh_init(Mesh* mesh, const VertexArray* vertices, const U32Array* indices);
// TODO(szulf): probably switch to some sort of a queue later on
static void mesh_draw(const Mesh* mesh);
static Error mesh_from_obj(Mesh* out, Arena* perm_arena, Arena* temp_arena, const char* path);

typedef struct MeshArray
{
  usize cap;
  usize len;
  Mesh* items;
} MeshArray;

typedef struct Model
{
  MeshArray meshes;
  Mat4  model;
} Model;

// TODO(szulf): probably switch to some sort of a queue later on
static void model_draw(const Model* model, Shader shader);
static void model_rotate(Model* model, f32 deg, const Vec3* axis);

// TODO(szulf): is this a good name
typedef struct Drawable
{
  Model model;
  Shader shader;
} Drawable;

typedef struct DrawableArray
{
  usize cap;
  usize len;
  Drawable* items;
} DrawableArray;

typedef struct Scene
{
  DrawableArray drawables;
  Mat4 view;
  Mat4 proj;
} Scene;

// TODO(szulf): probably switch to some sort of a queue later on
static void scene_draw(const Scene* scene);

static Error setup_shader(u32* out, Arena* arena, const char* path, ShaderType shader_type);
static Error link_shaders(u32* out, u32 vertex_shader, u32 fragment_shader);
static Error setup_shaders(Arena* arena);

#endif
