#ifndef MESH_H
#define MESH_H

typedef usize MeshHandle;
struct Mesh
{
  u32 vao;
  u32 vbo;
  u32 ebo;
  Array<Vertex> vertices;
  Array<u32> indices;
  MaterialHandle material;
};

Mesh mesh_make(const Array<Vertex>& vertices, const Array<u32>& indices, MaterialHandle material);

#endif
