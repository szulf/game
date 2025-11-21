#include "engine/renderer/mesh.hpp"

#ifdef GAME_OPENGL
#  include "gl_functions.hpp"
#else
#  error Unknown rendering backend
#endif

namespace core {

#ifdef GAME_OPENGL

Mesh Mesh::make(
  const btl::List<Vertex>& vertices,
  const btl::List<btl::u32>& indices,
  const btl::String& material_name
) {
  Mesh out = {};
  out.vertices = vertices;
  out.indices = indices;
  out.material_name = material_name;

  glGenVertexArrays(1, &out.backend_data.vao);
  glBindVertexArray(out.backend_data.vao);

  glGenBuffers(1, &out.backend_data.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, out.backend_data.vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    static_cast<GLsizei>(out.vertices.size * sizeof(Vertex)),
    out.vertices.data,
    GL_STATIC_DRAW
  );
  glGenBuffers(1, &out.backend_data.ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.backend_data.ebo);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    static_cast<GLsizei>(out.indices.size * sizeof(btl::u32)),
    out.indices.data,
    GL_STATIC_DRAW
  );

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  return out;
}

#else
#  error Unknown rendering backend
#endif

}
