#ifndef RENDERER_H
#define RENDERER_H

static void setup_renderer();
static void clear_screen();

enum class ShaderType : u8
{
  VERTEX,
  FRAGMENT,
};

enum class Shader : u8
{
  // NOTE(szulf): this has to be last
  DEFAULT,
};

u32 shader_map[(usize) Shader::DEFAULT + 1];

static void setup_shaders(mem::Arena& arena, Error* err);

struct Texture;

struct Material;

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
  Mat4 mat;

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw(Shader shader) const;

  void rotate(f32 deg, const Vec3& axis);
};


struct Renderable
{
  Model model;
  Shader shader;
};

struct Scene
{
  Array<Renderable> renderables;
  Mat4 view;
  Mat4 proj;

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw() const;
};


#ifdef GAME_OPENGL
#include "ogl_renderer.cpp"
#endif

#endif
