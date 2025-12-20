#ifndef MATERIAL_H
#define MATERIAL_H

namespace assets
{

typedef usize MaterialHandle;
struct Material
{
  bool wireframe;

  TextureHandle texture;
  ShaderHandle shader;
};

}

#endif
