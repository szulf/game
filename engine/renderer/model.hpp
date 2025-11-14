#pragma once

#include <filesystem>
#include <vector>

#include "mesh.hpp"

namespace core {

struct Model final {
  constexpr Model(std::vector<Mesh>&& m) noexcept : meshes{std::move(m)} {}
  // TODO(szulf): on fail, load an error model, do not throw exceptions
  Model(const std::filesystem::path& path);

  std::vector<Mesh> meshes{};
  math::mat4 matrix{};
};

}
