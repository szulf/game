#include "engine/renderer/shaders.hpp"

#include "engine/renderer/gl_functions.hpp"
#include "badtl/files.hpp"

namespace core {

#ifdef GAME_OPENGL

namespace shader_impl {

static btl::Result<btl::u32, ShaderError> setup_shader(const char* path, ShaderType shader_type) {
  auto scratch_arena = btl::ScratchArena::get();
  defer(scratch_arena.release());
  auto file = btl::read_file(path, scratch_arena.allocator);

  btl::u32 shader;
  switch (shader_type) {
    case ShaderType::Vertex: {
      shader = glCreateShader(GL_VERTEX_SHADER);
    } break;

    case ShaderType::Fragment: {
      shader = glCreateShader(GL_FRAGMENT_SHADER);
    } break;
  }

  auto* shader_src = btl::String::make(static_cast<const char*>(file.ptr), file.size).c_string(scratch_arena.allocator);
  glShaderSource(shader, 1, &shader_src, nullptr);
  glCompileShader(shader);
  GLint compiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled != GL_TRUE) {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetShaderInfoLog(shader, 1024, &log_length, message);
    switch (shader_type) {
      case ShaderType::Vertex: {
        btl::print("vertex shader compilation failed with message:\n{}\n", message);
        return btl::err<btl::u32>(ShaderError::InvalidVertex);
      } break;
      case ShaderType::Fragment: {
        btl::print("fragment shader compilation failed with message:\n{}\n", message);
        return btl::err<btl::u32>(ShaderError::InvalidFragment);
      } break;
    }
  }
  return btl::ok<ShaderError>(shader);
}

static btl::Result<btl::u32, ShaderError> link_shaders(btl::u32 vertex_shader, btl::u32 fragment_shader) {
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  GLint program_linked;
  glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
  if (program_linked != GL_TRUE) {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetProgramInfoLog(program, 1024, &log_length, message);
    return btl::err<btl::u32>(ShaderError::CouldntLink);
  }
  return btl::ok<ShaderError>(program);
}

}

ShaderMap ShaderMap::make() {
  ShaderMap sm;
  {
    btl::u32 v_shader =
      shader_impl::setup_shader("shaders/shader.vert", ShaderType::Vertex).expect("invalid vertex shader");
    btl::u32 f_shader =
      shader_impl::setup_shader("shaders/shader.frag", ShaderType::Fragment).expect("invalid fragment shader");
    btl::u32 shader = shader_impl::link_shaders(v_shader, f_shader).expect("couldnt link shaders");
    sm.map[static_cast<btl::usize>(Shader::Default)] = shader;
  }
  return sm;
}

#else
#  error Unknown rendering backend
#endif
}
