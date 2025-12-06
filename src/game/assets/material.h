#ifndef MATERIAL_H
#define MATERIAL_H

typedef usize MaterialHandle;
struct Material
{
  TextureHandle texture;
  ShaderHandle shader;
};

Material& assets_get_material(MaterialHandle handle);
MaterialHandle assets_set_material(const Material& material);

#endif
