#ifndef MATERIAL_H
#define MATERIAL_H

namespace assets
{

typedef usize MaterialHandle;
struct Material
{
  vec3 ambient_color;
  vec3 diffuse_color;
  vec3 specular_color;
  f32 specular_exponent;

  bool wireframe;

  TextureHandle diffuse_map;
  ShaderHandle shader;
};

}

#endif
