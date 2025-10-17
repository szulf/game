#ifndef RENDERER_H
#define RENDERER_H

static void setup_renderer(void);
static void clear_screen(void);

typedef enum ShaderType
{
  SHADER_TYPE_VERTEX,
  SHADER_TYPE_FRAGMENT,
} ShaderType;

typedef enum Shader
{
  // NOTE(szulf): this has to be last
  SHADER_DEFAULT,
} Shader;

u32 shader_map[SHADER_DEFAULT + 1];

static void setup_shaders(Arena* arena, Error* err);

typedef struct Texture Texture;

static Texture texture_make(Image* img);

typedef struct Material Material;

typedef struct Mesh Mesh;

typedef struct Vertex
{
  Vec3 pos;
  Vec3 normal;
  Vec2 uv;
} Vertex;

static Mesh mesh_make(Array<Vertex>* vertices, Array<u32>* indices, Material* material);
// TODO(szulf): probably switch to some sort of a queue later on
static void mesh_draw(const Mesh* mesh, Shader shader);

typedef struct Model
{
  Array<Mesh> meshes;
  Mat4 model;
} Model;

// TODO(szulf): probably switch to some sort of a queue later on
static void model_draw(const Model* model, Shader shader);
static void model_rotate(Model* model, f32 deg, const Vec3* axis);

typedef struct Renderable
{
  Model model;
  Shader shader;
} Renderable;

typedef struct Scene
{
  Array<Renderable> renderables;
  Mat4 view;
  Mat4 proj;
} Scene;

// TODO(szulf): probably switch to some sort of a queue later on
static void scene_draw(const Scene* scene);

#ifdef GAME_OPENGL
#include "ogl_renderer.cpp"
#endif

#endif
