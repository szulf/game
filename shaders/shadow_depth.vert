#version 330 core

layout(location = 0) in vec3 a_pos;
layout(location = 3) in mat4 a_model;

void main()
{
  gl_Position = a_model * vec4(a_pos, 1.0f);
}
