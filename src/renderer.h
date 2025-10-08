#ifndef RENDERER_H
#define RENDERER_H

static void setup_renderer();
static void clear_screen();

enum class ShaderType : u8
{
  Vertex,
  Fragment,
};

enum class Shader : u8
{
  // NOTE(szulf): this has to be last
  Default,
};

u32 shader_map[(usize) Shader::Default + 1];

static void setup_shaders(Arena* arena, Error* err);

struct Texture;

struct Material;

static Array<Material> g_materials;

inline static void setup_global_materials(Arena* perm_arena, Error* err);

struct Mesh;

struct Vertex
{
  Vec3 pos;
  Vec3 normal;
  Vec2 uv;
};

struct Model
{
  Array<Mesh> meshes;
  Mat4 model;

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw(Shader shader) const;
  void rotate(f32 deg, const Vec3* axis);
};

struct Drawable
{
  Model model;
  Shader shader;
};

struct Scene
{
  Array<Drawable> drawables;
  Mat4 view;
  Mat4 proj;

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw() const;
};

#ifdef GAME_OPENGL
#include "ogl_renderer.cpp"
#endif

#endif
