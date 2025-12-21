#version 460 core

// TODO(szulf): how do i differentiate between the diffuse color and diffuse texture? or do i just always use both?
struct Material
{
  vec3 ambient;
  vec3 diffuse;
  sampler2D diffuse_map;
  vec3 specular;
  float specular_exponent;
};

struct Light
{
  vec3 pos;
  vec3 color;

  // NOTE(szulf): for attenuation
  float constant;
  float linear;
  float quadratic;
};

in vec2 uv;
in vec3 normal;
in vec3 frag_pos;

out vec4 out_color;

uniform Material material;
uniform Light light;
uniform vec3 view_pos;

void main()
{
  vec3 norm = normalize(normal);
  vec3 object_color = vec3(texture(material.diffuse_map, uv)) + material.diffuse;

  float dist = length(light.pos - frag_pos);
  float attenuation = 1.0f / (light.constant + light.linear * dist + light.quadratic * dist * dist);

  // TODO(szulf): should ambient be multiplied by the light color?
  vec3 ambient = material.ambient;
  // TODO(szulf): am i really supposed to multiply ambient by attenuation?
  // isnt the point of ambient thats is the same everywhere in the scene?
  // ambient *= attenuation;

  vec3 light_dir = normalize(light.pos - frag_pos);
  float diff = max(dot(norm, light_dir), 0.0f);
  vec3 diffuse = diff * light.color;
  diffuse *= attenuation;

  float specular_strength = 0.5f;
  vec3 view_dir = normalize(view_pos - frag_pos);
  vec3 halfway = normalize(light_dir + view_dir);
  float spec = pow(max(dot(normal, halfway), 0.0f), material.specular_exponent);
  vec3 specular = spec * specular_strength * light.color;
  specular *= attenuation;

  vec3 color = (ambient + diffuse + specular) * object_color;
  out_color = vec4(color, 1.0f);
}
