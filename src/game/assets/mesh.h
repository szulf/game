#ifndef MESH_H
#define MESH_H

namespace assets
{

typedef usize MeshHandle;
struct Mesh
{
  Array<Vertex> vertices;
  Array<u32> indices;
  RenderingPrimitive primitive;
};

}

#endif
