#ifndef RENDERER_H
#define RENDERER_H

// TODO(szulf): i hate all these friends

class Texture;
class Material;
class Mesh;

class Renderer;

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

class ShaderMap
{
public:
  ShaderMap();

  inline u32& operator[](Shader shader)
  {
    return map[static_cast<usize>(shader)];
  }

  inline u32 operator[](Shader shader) const
  {
    return map[static_cast<usize>(shader)];
  }

private:
  u32 map[static_cast<usize>(Shader::Default) + 1]{};
};

struct Vertex
{
  math::vec3 pos;
  math::vec3 normal;
  math::vec2 uv;

  bool operator==(const Vertex& other) const;
};

#ifdef GAME_OPENGL
#  include "ogl_renderer.h"
#endif

struct Model
{
  Model(const math::mat4& mat) : mat{mat} {}
  void rotate(f32 deg, const math::vec3& axis);

  std::vector<Mesh> meshes{};
  math::quat rotation{};
  math::quat visible_rotation{};
  math::mat4 mat{1.0f};
};

struct Renderable
{
  Model model;
  Shader shader;
};

class Scene
{
public:
  Scene(std::vector<Renderable>&& r) : renderables{std::move(r)} {}
  void translate();

public:
  // TODO(szulf): should these be private in the future?
  std::vector<Renderable> renderables{};
  math::mat4 view{1.0f};
  math::mat4 proj{1.0f};

  friend Renderer;
};

class Renderer
{
public:
  Renderer(ShaderMap& shader_map);

  auto clear_screen() -> void;

  auto render(const Scene& scene) -> void;
  auto render(const Model& model, Shader shader) -> void;
  auto render(const Mesh& mesh, Shader shader) -> void;

private:
  ShaderMap& shader_map;
};

#ifdef GAME_OPENGL
#  include "ogl_renderer.cpp"
#endif

#endif
