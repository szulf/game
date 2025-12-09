#ifndef MODEL_H
#define MODEL_H

typedef usize ModelHandle;
struct Model
{
  Array<MeshHandle> meshes;
  Mat4 matrix;
};

#endif
