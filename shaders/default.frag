#version 460 core

struct Material
{
  vec3 ambient;
  vec3 diffuse;
  sampler2D diffuse_map;
  vec3 specular;
  float specular_exponent;
};

in vec2 uv;

out vec4 color;

uniform Material material;
uniform bool light_bulb_on;

void main()
{
  color = texture(material.diffuse_map, uv) + vec4(material.diffuse, 1.0f);
}
