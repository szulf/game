#ifndef MESH_H
#define MESH_H

namespace assets
{

enum Primitive
{
  PRIMITIVE_TRIANGLES,
  PRIMITIVE_LINE_STRIP,
};

typedef usize MeshHandle;
struct Mesh
{
  u32 vao;
  u32 vbo;
  u32 ebo;
  Primitive primitive;
  usize index_count;

  // NOTE(szulf): needed to calculate bounding boxes dynamically later on
  Array<Vertex> vertices;
};

Mesh mesh_make(const Array<Vertex>& vertices, const Array<u32>& indices, Primitive primitive);

}

#endif
