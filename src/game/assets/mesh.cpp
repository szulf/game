#include "mesh.h"

// TODO(szulf): this is horrible
namespace renderer
{
static u32 instancing_matrix_buffer;
struct InstanceData
{
  mat4 model;
  vec3 tint;
};
}

namespace assets
{

#ifdef RENDERER_OPENGL

Mesh mesh_make(const Array<Vertex>& vertices, const Array<u32>& indices, Primitive primitive)
{
  Mesh out = {};
  out.vertices = vertices;
  out.index_count = indices.size;
  out.primitive = primitive;

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
    (GLsizei) (indices.size * sizeof(u32)),
    indices.data,
    GL_STATIC_DRAW
  );

  {
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
    rendering.glVertexAttribPointer(
      2,
      2,
      GL_FLOAT,
      GL_FALSE,
      sizeof(Vertex),
      (void*) offsetof(Vertex, uv)
    );
    rendering.glEnableVertexAttribArray(2);
  }

  {
    rendering.glBindBuffer(GL_ARRAY_BUFFER, renderer::instancing_matrix_buffer);
    // NOTE(szulf): model matrix
    rendering.glVertexAttribPointer(
      3,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(renderer::InstanceData),
      (void*) offsetof(renderer::InstanceData, model)
    );
    rendering.glEnableVertexAttribArray(3);
    rendering.glVertexAttribPointer(
      4,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(renderer::InstanceData),
      (void*) (offsetof(renderer::InstanceData, model) + sizeof(vec4))
    );
    rendering.glEnableVertexAttribArray(4);
    rendering.glVertexAttribPointer(
      5,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(renderer::InstanceData),
      (void*) (offsetof(renderer::InstanceData, model) + 2 * sizeof(vec4))
    );
    rendering.glEnableVertexAttribArray(5);
    rendering.glVertexAttribPointer(
      6,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(renderer::InstanceData),
      (void*) (offsetof(renderer::InstanceData, model) + 3 * sizeof(vec4))
    );
    rendering.glEnableVertexAttribArray(6);
    // NOTE(szulf): entity tint
    rendering.glVertexAttribPointer(
      7,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(renderer::InstanceData),
      (void*) offsetof(renderer::InstanceData, tint)
    );
    rendering.glEnableVertexAttribArray(7);
  }

  rendering.glVertexAttribDivisor(3, 1);
  rendering.glVertexAttribDivisor(4, 1);
  rendering.glVertexAttribDivisor(5, 1);
  rendering.glVertexAttribDivisor(6, 1);
  rendering.glVertexAttribDivisor(7, 1);

  rendering.glBindVertexArray(0);

  return out;
}

#endif

}
