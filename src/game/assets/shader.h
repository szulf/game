#ifndef SHADER_H
#define SHADER_H

typedef u32 Shader;
enum ShaderHandle
{
  SHADER_GREEN,
  SHADER_YELLOW,
  SHADER_DEFAULT,
};

// TODO(szulf): switch to a game error struct instead?
enum ShaderError
{
  SHADER_ERROR_COMPILATION = GLOBAL_ERROR_COUNT,
  SHADER_ERROR_LINKING,
};

ShaderHandle assets_set_shader(Shader shader);
Shader assets_get_shader(ShaderHandle handle);

#endif
