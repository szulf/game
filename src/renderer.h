#ifndef RENDERER_H
#define RENDERER_H

namespace game
{

enum class ShaderType : u8
{
  Vertex,
  Fragment,
};

enum class Shader : u8
{
  // NOTE(szulf): this has to be last
  DefaultShader,
};

struct Renderer
{
  static void clear_screen();
};

u32 shader_map[static_cast<i32>(Shader::DefaultShader) + 1];

}

#ifdef GAME_OPENGL
#include "ogl_renderer.cpp"
#endif

#endif
