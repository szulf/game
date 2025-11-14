#version 460 core

out vec4 color;

in vec2 uv;

uniform sampler2D sampler;

void main()
{
  color = texture(sampler, uv);
}
