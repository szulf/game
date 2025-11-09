#pragma once

#include <filesystem>

#include "mesh.hpp"

namespace core
{

class Model
{
public:
  Model(std::vector<Mesh>&& meshes) : m_meshes{std::move(meshes)} {}
  // NOTE(szulf): on fail load an error model, do not throw exceptions
  Model(const std::filesystem::path& path);

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

}
