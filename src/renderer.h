#ifndef RENDERER_H
#define RENDERER_H

typedef enum ShaderType
{
  SHADER_TYPE_VERTEX,
  SHADER_TYPE_FRAGMENT,
} ShaderType;

typedef enum Shader
{
  // NOTE(szulf): this has to be last
  SHADER_DEFAULT,
} Shader;

static void clear_screen();

u32 shader_map[SHADER_DEFAULT + 1];

#ifdef GAME_OPENGL
#include "ogl_renderer.c"
#endif

#endif
