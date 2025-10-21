#include "ogl_renderer.h"

#include "ogl_functions.h"

static void
setup_renderer()
{
  glEnable(GL_DEPTH_TEST);
}

static void
clear_screen()
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

Mesh
Mesh::make(const Array<Vertex>& vertices, const Array<u32>& indices, const Material& mat)
{
  Mesh mesh;
  mesh.vertices = vertices;
  mesh.indices = indices;
  mesh.material = mat;

  glGenVertexArrays(1, &mesh.vao);
  glBindVertexArray(mesh.vao);

  u32 vertex_vbo;
  u32 vertex_ebo;
  glGenBuffers(1, &vertex_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
  glBufferData(GL_ARRAY_BUFFER, (GLsizei) (mesh.vertices.len * sizeof(Vertex)),
               mesh.vertices.items, GL_STATIC_DRAW);
  glGenBuffers(1, &vertex_ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei) (mesh.indices.len * sizeof(u32)),
               mesh.indices.items, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (3 * sizeof(f32)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (6 * sizeof(f32)));
  glEnableVertexAttribArray(2);

  return mesh;
}

void
Mesh::draw(Shader shader) const
{
  // TODO(szulf): this should not happen for every mesh,
  // should also check how many textures should be made active
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, material.texture.id);
  glUniform1i(glGetUniformLocation(shader_map[(usize) shader], "sampler"), 0);
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, (GLsizei) indices.len, GL_UNSIGNED_INT, 0);
}

void
Model::draw(Shader shader) const
{
  glUniformMatrix4fv(glGetUniformLocation(shader_map[(usize) shader], "model"), 1, false, mat.data);

  for (const auto& mesh : meshes)
  {
    mesh.draw(shader);
  }
}

void
Model::rotate(f32 deg, const Vec3& axis)
{
  mat.rotate(radians(deg), axis);
}

void
Scene::draw() const
{
  for (const auto& renderable : renderables)
  {
    glUseProgram(shader_map[(usize) renderable.shader]);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[(usize) renderable.shader], "view"),
                       1, false, view.data);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[(usize) renderable.shader], "proj"),
                       1, false, proj.data);

    renderable.model.draw(renderable.shader);
  }
}

static u32
setup_shader(mem::Arena& arena, const char* path, ShaderType shader_type, Error* err)
{
  Error error = Error::SUCCESS;
  void* shader_src = os::read_entire_file(path, arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, (u32) -1);

  u32 shader;
  switch (shader_type)
  {
    case ShaderType::VERTEX:
    {
      shader = glCreateShader(GL_VERTEX_SHADER);
    } break;

    case ShaderType::FRAGMENT:
    {
      shader = glCreateShader(GL_FRAGMENT_SHADER);
    } break;
  }

  glShaderSource(shader, 1, (const char* const*) &shader_src, 0);
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
      case ShaderType::VERTEX:
      {
        LOG("Error compiling vertex shader:\n{}", message);
        *err = Error::SHADER_COMPILATION;
        return (u32) -1;
      } break;
      case ShaderType::FRAGMENT:
      {
        LOG("Error compiling fragment shader:\n{}", message);
        *err = Error::SHADER_COMPILATION;
        return (u32) -1;
      } break;
    }
  }

  *err = Error::SUCCESS;
  return shader;
}

static u32
link_shaders(u32 vertex_shader, u32 fragment_shader, Error* err)
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
    LOG("Error compiling fragment shader:\n{}", message);
    *err = Error::SHADER_LINKING;
    return (u32) -1;
  }

  *err = Error::SUCCESS;
  return program;
}

static void
setup_shaders(mem::Arena& arena, Error* err)
{
  Error error = Error::SUCCESS;

  {
    u32 v_shader = setup_shader(arena, "src/shader.vert", ShaderType::VERTEX, &error);
    ERROR_ASSERT(error == Error::SUCCESS, *err, error,);

    u32 f_shader = setup_shader(arena, "src/shader.frag", ShaderType::FRAGMENT, &error);
    ERROR_ASSERT(error == Error::SUCCESS, *err, error,);

    u32 shader = link_shaders(v_shader, f_shader, &error);
    ERROR_ASSERT(error == Error::SUCCESS, *err, error,);

    shader_map[(usize) Shader::DEFAULT] = shader;
  }

  *err = Error::SUCCESS;
}

Texture
Texture::make(const png::Image& img)
{
  Texture texture = {0};

  glGenTextures(1, &texture.id);
  glBindTexture(GL_TEXTURE_2D, texture.id);

  // TODO(szulf): do i want to customize these
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei) img.width, (GLsizei) img.height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, img.data);
  glGenerateMipmap(GL_TEXTURE_2D);

  return texture;
}
