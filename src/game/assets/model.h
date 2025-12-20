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
};

}

#endif
