#ifndef SHADER_H
#define SHADER_H

namespace assets
{

typedef u32 Shader;
enum ShaderHandle
{
  SHADER_DEFAULT,
  SHADER_GREEN,
  SHADER_YELLOW,

  SHADER_COUNT,
};

// TODO(szulf): switch to a game error struct instead?
enum ShaderError
{
  SHADER_ERROR_COMPILATION = GLOBAL_ERROR_COUNT,
  SHADER_ERROR_LINKING,
};

}

#endif
