#pragma once

#include <vector>
#include <cstdint>
#include <array>
#include <memory>

#include "math.hpp"

namespace core
{

enum class Shader : std::uint8_t
{
  // NOTE(szulf): this should be last
  Default,
};

class ShaderMap
{
public:
  ShaderMap();
  inline auto operator[](Shader shader) const -> std::uint32_t
  {
    return m_map[static_cast<std::size_t>(shader)];
  }
  inline static auto getInstance() -> ShaderMap&
  {
    static ShaderMap map{};
    return map;
  }

private:
  std::array<std::uint32_t, static_cast<std::size_t>(Shader::Default) + 1> m_map{};
};

class Texture
{
private:
  struct BackendData;

public:
  Texture();
  ~Texture();

  inline auto backendData() const -> BackendData&
  {
    return *m_backend_data;
  }

private:
  // TODO(szulf): can i somehow get rid of the dynamic allocation here?
  std::unique_ptr<BackendData> m_backend_data{};
};

struct Material
{
  Texture texture;
};

struct Vertex
{
  math::vec3 position{};
  math::vec3 normal{};
  math::vec2 uv{};
};

class Mesh
{
private:
  struct BackendData;

public:
  Mesh();
  ~Mesh();

  inline auto material() const -> const Material&
  {
    return m_material;
  }
  inline auto indices() const -> const std::vector<std::uint32_t>&
  {
    return m_indices;
  }
  inline auto vertices() const -> const std::vector<Vertex>&
  {
    return m_vertices;
  }
  inline auto backendData() const -> BackendData&
  {
    return *m_backend_data;
  }

private:
  Material m_material;
  std::vector<Vertex> m_vertices;
  std::vector<std::uint32_t> m_indices;

  // TODO(szulf): can i somehow get rid of the dynamic allocation here?
  std::unique_ptr<BackendData> m_backend_data{};
};

class Model
{
public:
  Model(std::vector<Mesh>&& meshes) : m_meshes{std::move(meshes)} {}
  inline auto matrix() const -> const math::mat4&
  {
    return m_matrix;
  }
  inline auto meshes() const -> const std::vector<Mesh>&
  {
    return m_meshes;
  }

private:
  std::vector<Mesh> m_meshes{};
  math::mat4 m_matrix;
};

struct Renderable
{
  Model model;
  Shader shader;
};

class Scene
{
public:
  inline auto getRenderables() const -> const std::vector<Renderable>&
  {
    return m_renderables;
  }

  inline auto view() const -> const math::mat4&
  {
    return m_view;
  }
  inline auto projection() const -> const math::mat4&
  {
    return m_proj;
  }

private:
  std::vector<Renderable> m_renderables{};
  math::mat4 m_view{};
  math::mat4 m_proj{};
};

namespace renderer
{

void init();
void clearScreen();
void render(const Scene& scene);

}

}
