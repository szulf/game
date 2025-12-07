#ifndef MATERIAL_H
#define MATERIAL_H

typedef usize MaterialHandle;
struct Material
{
  TextureHandle texture;
  ShaderHandle shader;
};

#endif
