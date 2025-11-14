#include "renderer/mesh.hpp"

#ifdef GAME_OPENGL
#  include "gl_functions.hpp"
#else
#  error Unknown rendering backend
#endif

namespace core {

#ifdef GAME_OPENGL

Mesh::Mesh(std::vector<Vertex>&& v, std::vector<std::uint32_t>&& i, std::string&& mat) noexcept
  : vertices{std::move(v)}, indices{std::move(i)}, material_name{std::move(mat)} {
  glGenVertexArrays(1, &backend_data.vao);
  glBindVertexArray(backend_data.vao);

  glGenBuffers(1, &backend_data.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, backend_data.vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    static_cast<GLsizei>(vertices.size() * sizeof(Vertex)),
    vertices.data(),
    GL_STATIC_DRAW
  );
  glGenBuffers(1, &backend_data.ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backend_data.ebo);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    static_cast<GLsizei>(indices.size() * sizeof(std::uint32_t)),
    indices.data(),
    GL_STATIC_DRAW
  );

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));
  glEnableVertexAttribArray(2);
}

Mesh::~Mesh() {
  if (backend_data.vbo != 0 && backend_data.ebo != 0 && backend_data.vao != 0) {
    glDeleteBuffers(1, &backend_data.vbo);
    glDeleteBuffers(1, &backend_data.ebo);
    glDeleteVertexArrays(1, &backend_data.vao);
  }
}

Mesh::Mesh(Mesh&& other)
  : vertices{std::move(other.vertices)}, indices{std::move(other.indices)},
    material_name{std::move(other.material_name)}, backend_data{other.backend_data} {
  other.backend_data = {};
}

Mesh& Mesh::operator=(Mesh&& other) {
  vertices = std::move(other.vertices);
  indices = std::move(other.indices);
  material_name = std::move(other.material_name);

  backend_data = other.backend_data;
  other.backend_data = {};
  return *this;
}

#else
#  error Unknown rendering backend
#endif

}
