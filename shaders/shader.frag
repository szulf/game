#version 460 core

out vec4 color;

in vec2 uv;

uniform sampler2D sampler;
uniform bool emissive;

void main()
{
  // NOTE(szulf): this is just a proof of concept for now, it will definitely change later when making
  // the light bulb(or other model) actually look nice
  color = vec4(1.0f, 1.0f, 0.0f, 1.0f) * float(emissive) + texture(sampler, uv) * float(!emissive);
}
