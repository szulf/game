#include "ogl_renderer.h"

#include "ogl_functions.h"

Renderer::Renderer(ShaderMap& shader_map) : shader_map{shader_map}
{
  glEnable(GL_DEPTH_TEST);
}

void Renderer::clear_screen()
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::render(const Scene& scene)
{
  for (const auto& renderable : scene.renderables)
  {
    glUseProgram(shader_map[renderable.shader]);
    glUniformMatrix4fv(
      glGetUniformLocation(shader_map[renderable.shader], "view"),
      1,
      false,
      scene.view.data
    );
    glUniformMatrix4fv(
      glGetUniformLocation(shader_map[renderable.shader], "proj"),
      1,
      false,
      scene.proj.data
    );

    render(renderable.model, renderable.shader);
  }
}

void Renderer::render(const Model& model, Shader shader) {}

void Renderer::render(const Mesh& mesh, Shader shader)
{
  // TODO(szulf): this should not happen for every mesh,
  // should also check how many textures should be made active
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mesh.material.texture.id);
  glUniform1i(glGetUniformLocation(shader_map[shader], "sampler"), 0);
  glBindVertexArray(mesh.vao);
  glDrawElements(GL_TRIANGLES, (GLsizei) mesh.indices.size(), GL_UNSIGNED_INT, 0);
}

Mesh::Mesh(std::vector<Vertex>&& v, std::vector<u32>&& i, const Material& m)
  : vertices{v}, indices{i}, material{m}
{
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  u32 vertex_vbo;
  u32 vertex_ebo;
  glGenBuffers(1, &vertex_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    (GLsizei) (vertices.size() * sizeof(Vertex)),
    vertices.data(),
    GL_STATIC_DRAW
  );
  glGenBuffers(1, &vertex_ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_ebo);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    (GLsizei) (indices.size() * sizeof(u32)),
    indices.data(),
    GL_STATIC_DRAW
  );

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (3 * sizeof(f32)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (6 * sizeof(f32)));
  glEnableVertexAttribArray(2);
}

static u32 setup_shader(const std::filesystem::path& path, ShaderType shader_type)
{
  std::ifstream shader_stream{path};
  std::stringstream ss{};
  ss << shader_stream.rdbuf();
  std::string shader_src{ss.str()};

  u32 shader{};
  switch (shader_type)
  {
    case ShaderType::Vertex:
    {
      shader = glCreateShader(GL_VERTEX_SHADER);
    }
    break;

    case ShaderType::Fragment:
    {
      shader = glCreateShader(GL_FRAGMENT_SHADER);
    }
    break;
  }

  const char* const ssrc = shader_src.data();
  glShaderSource(shader, 1, &ssrc, 0);
  glCompileShader(shader);
  GLint compiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetShaderInfoLog(shader, 1024, &log_length, message);
    switch (shader_type)
    {
      case ShaderType::Vertex:
      {
        LOG("Error compiling vertex shader:\n{}", message);
        throw std::runtime_error{"shader compilation"};
      }
      break;
      case ShaderType::Fragment:
      {
        LOG("Error compiling fragment shader:\n{}", message);
        throw std::runtime_error{"shader compilation"};
      }
      break;
    }
  }
  return shader;
}

static u32 link_shaders(u32 vertex_shader, u32 fragment_shader)
{
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  GLint program_linked;
  glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
  if (program_linked != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetProgramInfoLog(program, 1024, &log_length, message);
    LOG("Error linking shader:\n{}", message);
    throw std::runtime_error{"shader linking"};
  }
  return program;
}

ShaderMap::ShaderMap()
{
  {
    u32 v_shader = setup_shader("src/shader.vert", ShaderType::Vertex);
    u32 f_shader = setup_shader("src/shader.frag", ShaderType::Fragment);
    u32 shader = link_shaders(v_shader, f_shader);
    (*this)[Shader::Default] = shader;
  }
}

Texture::Texture(const png::Image& img)
{
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);

  // TODO(szulf): do i want to customize these
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA8,
    (GLsizei) img.width,
    (GLsizei) img.height,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    img.data
  );
  glGenerateMipmap(GL_TEXTURE_2D);
}
