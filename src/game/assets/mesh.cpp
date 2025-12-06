#include "mesh.h"

Mesh mesh_make(const Array<Vertex>& vertices, const Array<u32>& indices, MaterialHandle material)
{
  Mesh out = {};
  out.vertices = vertices;
  out.indices = indices;
  out.material = material;

  rendering.glGenVertexArrays(1, &out.vao);
  rendering.glBindVertexArray(out.vao);

  rendering.glGenBuffers(1, &out.vbo);
  rendering.glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
  rendering.glBufferData(
    GL_ARRAY_BUFFER,
    (GLsizei) (out.vertices.size * sizeof(Vertex)),
    out.vertices.data,
    GL_STATIC_DRAW
  );
  rendering.glGenBuffers(1, &out.ebo);
  rendering.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
  rendering.glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    (GLsizei) (out.indices.size * sizeof(u32)),
    out.indices.data,
    GL_STATIC_DRAW
  );

  rendering.glVertexAttribPointer(
    0,
    3,
    GL_FLOAT,
    GL_FALSE,
    sizeof(Vertex),
    (void*) offsetof(Vertex, position)
  );
  rendering.glEnableVertexAttribArray(0);
  rendering.glVertexAttribPointer(
    1,
    3,
    GL_FLOAT,
    GL_FALSE,
    sizeof(Vertex),
    (void*) offsetof(Vertex, normal)
  );
  rendering.glEnableVertexAttribArray(1);
  rendering
    .glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, uv));
  rendering.glEnableVertexAttribArray(2);

  return out;
}
