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

static void setup_shaders(Error* err);

struct Texture;

struct Material;

struct Mesh;

struct Vertex
{
  Vec3 pos;
  Vec3 normal;
  Vec2 uv;

  bool operator==(const Vertex& other) const;
  bool operator!=(const Vertex& other) const;
};

struct Model
{
  std::pmr::vector<Mesh> meshes;
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
  std::pmr::vector<Drawable> drawables;
  Mat4 view;
  Mat4 proj;

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw() const;
};

#ifdef GAME_OPENGL
#include "ogl_renderer.cpp"
#endif

#endif
