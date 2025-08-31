#ifndef OGL_RENDERER_H
#define OGL_RENDERER_H

namespace game
{

struct Vertex
{
  Vec3 pos;
};

struct Mesh
{
  Array<Vertex> vertices;
  Array<u32>    indices;

  u32 vao;
  u32 vbo;
  u32 ebo;

  Mesh() {}
  Mesh(const Array<Vertex>& v, const Array<u32>& i);

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw() const;

  static Result<Mesh> from_obj(mem::Arena& perm_arena, mem::Arena& temp_arena, const char* path);
};

struct Model
{
  Array<Mesh> meshes;
  Mat4  model;

  Model() {}
  Model(const Array<Mesh>& m) : meshes{m} {}

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw(Shader shader) const;

  void rotate(f32 deg, const Vec3& axis);
};

// TODO(szulf): is this a good name
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

  Scene() {}

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw() const;
};

static Result<u32> setup_shader(mem::Arena& arena, const char* filepath,
                                ShaderType shader_type);
static Result<u32> link_shaders(u32 vertex_shader, u32 fragment_shader);
static Result<void> setup_shaders(mem::Arena& arena);

}

#endif
