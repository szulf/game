#pragma once

#include <vector>

#include "renderer/vertex.hpp"
#include "renderer/material.hpp"

namespace core
{

class Mesh
{
private:
#ifdef GAME_OPENGL
  struct BackendData
  {
    std::uint32_t vao{};
  };
#else
#  error Unknown rendering backend
#endif

public:
  Mesh(std::vector<Vertex>&& vertices, std::vector<std::uint32_t>&& indices, Material&& material);

  inline auto material() const noexcept -> const Material&
  {
    return m_material;
  }
  inline auto indices() const noexcept -> const std::vector<std::uint32_t>&
  {
    return m_indices;
  }
  inline auto vertices() const noexcept -> const std::vector<Vertex>&
  {
    return m_vertices;
  }
  inline auto backendData() const noexcept -> const BackendData&
  {
    return m_backend_data;
  }

private:
  std::vector<Vertex> m_vertices;
  std::vector<std::uint32_t> m_indices;
  Material m_material;
  BackendData m_backend_data{};
};

}
