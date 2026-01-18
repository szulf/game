#version 330 core

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

  // NOTE: for attenuation
  float constant;
  float linear;
  float quadratic;
};

#define MAX_LIGHTS 32

in VERT_OUT
{
  vec2 uv;
  vec3 normal;
  vec3 frag_pos;
  vec3 tint;
}
vert_out;

out vec4 out_color;

uniform Material material;
uniform samplerCube shadow_map;
uniform float shadow_map_camera_far_plane;

layout(std140) uniform Camera
{
  mat4 proj_view;
  vec3 view_pos;
  float far_plane;
};

layout(std140) uniform Lights
{
  int light_count;
  Light lights[MAX_LIGHTS];
};

void main()
{
  vec3 norm = normalize(vert_out.normal);
  vec3 object_color =
    (vec3(texture(material.diffuse_map, vert_out.uv)) + material.diffuse) * vert_out.tint;

  vec3 diffuse_specular = vec3(0.0f);
  float shadow = 0.0f;
  for (int i = 0; i < light_count; ++i)
  {
    float dist = length(lights[i].pos - vert_out.frag_pos);
    float attenuation =
      1.0f / (lights[i].constant + lights[i].linear * dist + lights[i].quadratic * dist * dist);

    vec3 light_dir = normalize(lights[i].pos - vert_out.frag_pos);
    float diff = max(dot(norm, light_dir), 0.0f);
    vec3 diffuse = diff * lights[i].color;
    diffuse *= attenuation;

    float specular_strength = 0.5f;
    vec3 view_dir = normalize(view_pos - vert_out.frag_pos);
    vec3 halfway = normalize(light_dir + view_dir);
    float spec = pow(max(dot(norm, halfway), 0.0f), material.specular_exponent);
    vec3 specular = spec * specular_strength * lights[i].color;
    specular *= attenuation;

    vec3 frag_to_light = vert_out.frag_pos - lights[i].pos;
    float closest_depth = texture(shadow_map, frag_to_light).r;
    closest_depth *= shadow_map_camera_far_plane;
    float current_depth = length(frag_to_light);
    float bias = 0.05f;
    // TODO: if there would be more than 1 light could shadow be more than 1.0f?
    shadow += current_depth - bias > closest_depth ? 1.0f : 0.0f;

    diffuse_specular += diffuse + specular;
  }

  vec3 color = (material.ambient + (1.0f - shadow) * diffuse_specular) * object_color;
  out_color = vec4(color, 1.0f);
}
