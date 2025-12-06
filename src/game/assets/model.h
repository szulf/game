#ifndef MODEL_H
#define MODEL_H

typedef usize ModelHandle;
struct Model
{
  Array<MeshHandle> meshes;
  Mat4 matrix;
};

enum StaticModel
{
  STATIC_MODEL_BOUNDING_BOX = 1,
  STATIC_MODEL_RING,
};

void static_model_init(
  StaticModel static_model,
  ShaderHandle shader,
  const Array<Vertex>& vertices,
  const Array<u32>& indices,
  Allocator& allocator
);

#endif
