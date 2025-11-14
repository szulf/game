#pragma once

#include <vector>

#include "renderer/vertex.hpp"

namespace core {

struct Mesh final {
#ifdef GAME_OPENGL
  struct BackendData final {
    std::uint32_t vao{};
    std::uint32_t vbo{};
    std::uint32_t ebo{};
  };
#else
#  error Unknown rendering backend
#endif

  Mesh(std::vector<Vertex>&& vertices, std::vector<std::uint32_t>&& indices, std::string&& material_name) noexcept;
  ~Mesh();
  Mesh(const Mesh& other) = delete;
  Mesh& operator=(const Mesh& other) = delete;

  Mesh(Mesh&& other);
  Mesh& operator=(Mesh&& other);

  std::vector<Vertex> vertices{};
  std::vector<std::uint32_t> indices{};
  // NOTE(szulf): would prefer this to be a uuid but obj stores it as string so not much i can do ig
  std::string material_name{};
  BackendData backend_data{};
};

}
