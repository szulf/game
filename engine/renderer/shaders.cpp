#include "renderer/shaders.hpp"

#include <filesystem>
#include <fstream>

#include "gl_functions.hpp"

namespace core {

#ifdef GAME_OPENGL

namespace shader_impl {

static std::uint32_t setup_shader(const std::filesystem::path& path, ShaderType shader_type) {
  std::ifstream shader_stream{path};
  std::stringstream ss{};
  ss << shader_stream.rdbuf();
  std::string shader_src{ss.str()};

  std::uint32_t shader{};
  switch (shader_type) {
    case ShaderType::Vertex: {
      shader = glCreateShader(GL_VERTEX_SHADER);
    } break;

    case ShaderType::Fragment: {
      shader = glCreateShader(GL_FRAGMENT_SHADER);
    } break;
  }

  const char* const ssrc = shader_src.data();
  glShaderSource(shader, 1, &ssrc, nullptr);
  glCompileShader(shader);
  GLint compiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled != GL_TRUE) {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetShaderInfoLog(shader, 1024, &log_length, message);
    switch (shader_type) {
      case ShaderType::Vertex: {
        throw std::runtime_error{std::format("Error compiling vertex shader:\n{}", message)};
      } break;
      case ShaderType::Fragment: {
        throw std::runtime_error{std::format("Error compiling fragment shader:\n{}", message)};
      } break;
    }
  }
  return shader;
}

static std::uint32_t link_shaders(std::uint32_t vertex_shader, std::uint32_t fragment_shader) {
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
    throw std::runtime_error{std::format("Error linking shader:\n{}", message)};
  }
  return program;
}

}

ShaderMap::ShaderMap() {
  {
    std::uint32_t v_shader = shader_impl::setup_shader("shaders/shader.vert", ShaderType::Vertex);
    std::uint32_t f_shader = shader_impl::setup_shader("shaders/shader.frag", ShaderType::Fragment);
    std::uint32_t shader = shader_impl::link_shaders(v_shader, f_shader);
    m_map[static_cast<std::size_t>(Shader::Default)] = shader;
  }
}

#else
#  error Unknown rendering backend
#endif
}
