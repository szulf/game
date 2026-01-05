#version 330 core

struct Material
{
  vec3 ambient;
  vec3 diffuse;
  sampler2D diffuse_map;
  vec3 specular;
  float specular_exponent;
};

in VERT_OUT
{
  vec2 uv;
  vec3 normal;
  vec3 frag_pos;
  vec3 tint;
}
vert_out;

out vec4 color;

uniform Material material;

void main()
{
  vec3 object_color = (texture(material.diffuse_map, vert_out.uv).rgb + material.diffuse) * vert_out.tint;
  color = vec4(object_color, 1.0f);
}
