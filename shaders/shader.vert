#version 460 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

out vec2 uv;
out vec3 normal;
out vec3 frag_pos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
  gl_Position = proj * view * model * vec4(a_pos, 1.0f);
  uv = a_uv;
  frag_pos = vec3(model * vec4(a_pos, 1.0f));
  normal = mat3(transpose(inverse(model))) * a_normal;
}
