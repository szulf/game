#ifndef MODEL_H
#define MODEL_H

namespace assets
{

struct MeshMaterialPair
{
  MeshHandle mesh;
  MaterialHandle material;
};

typedef usize ModelHandle;
struct Model
{
  Array<MeshMaterialPair> parts;

  static ModelHandle from_file(const char* path, Allocator& allocator, Error& out_error);
};

}

#endif
