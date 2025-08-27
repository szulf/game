#include "ogl_renderer.h"

#include "glad/src/glad.c"

namespace game
{

void Renderer::clear_screen()
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

// TODO(szulf): i think these should be moved instead of copied here but i honestly dont care for now
Mesh::Mesh(const Array<Vertex>& v, const Array<u32>& i) : vertices{v}, indices{i}
{
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(vertices.len * sizeof(Vertex)), vertices.data,
               GL_STATIC_DRAW);

  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizei>(indices.len * sizeof(u32)),
               indices.data, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), reinterpret_cast<void*>(0));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::draw() const
{
  glBindVertexArray(vao);

  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.len), GL_UNSIGNED_INT, 0);
}

Mesh Mesh::from_obj(mem::Arena& arena, const char* path)
{
  // TODO(szulf): for now hardcoded, implement later
  (void) path;

  game::Vertex vertices[] = {
    {-0.5f, -0.5f, -0.5f},
    { 0.5f, -0.5f, -0.5f},
    { 0.5f,  0.5f, -0.5f},
    {-0.5f,  0.5f, -0.5f},
    {-0.5f, -0.5f,  0.5f},
    { 0.5f, -0.5f,  0.5f},
    { 0.5f,  0.5f,  0.5f},
    {-0.5f,  0.5f,  0.5f},
  };

  u32 indices[] = {
    0, 1, 2,
    2, 3, 0,
    4, 5, 6,
    6, 7, 4,
    4, 0, 3,
    3, 7, 4,
    1, 5, 6,
    6, 2, 1,
    4, 5, 1,
    1, 0, 4,
    3, 2, 6,
    6, 7, 3,
  };

  return {
    {
      arena,
      vertices,
      8,
    },
    {
      arena,
      indices,
      36,
    },
  };
}

void Model::draw(Shader shader)
{
  glUseProgram(shader_map[static_cast<usize>(shader)]);
  glUniformMatrix4fv(glGetUniformLocation(shader_map[static_cast<usize>(shader)], "model"),
                     1, false, model.data);

  for (const auto& mesh : meshes)
  {
    mesh.draw();
  }
}

void Model::rotateY(f32 deg)
{
  model.rotate(math::radians(deg), {0.0f, 1.0f, 0.0f});
}

Result<u32> setup_shader(mem::Arena& arena, const char* filepath,
                                     ShaderType shader_type)
{
  auto shader_src_result = platform::read_entire_file(arena, filepath);
  if (shader_src_result.has_error)
  {
    return {shader_src_result.err};
  }
  const char* shader_src = static_cast<char*>(shader_src_result.val);

  u32 shader;
  switch (shader_type)
  {
    case ShaderType::Vertex: {
      shader = glCreateShader(GL_VERTEX_SHADER);
      break;
    }

    case ShaderType::Fragment: {
      shader = glCreateShader(GL_FRAGMENT_SHADER);
      break;
    }
  }

  glShaderSource(shader, 1, &shader_src, nullptr);
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
      case ShaderType::Vertex: {
        fprintf(stderr, "Error compiling vertex shader:\n%s\n", message);
        return {Error::ShaderCompilation};
      }

      case ShaderType::Fragment: {
        fprintf(stderr, "Error compiling fragment shader:\n%s\n", message);
        return {Error::ShaderCompilation};
      }
    }
  }

  return {shader};
}

Result<u32> link_shaders(u32 vertex_shader, u32 fragment_shader)
{
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  GLint program_linked;
  glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
  if (program_linked != true)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetProgramInfoLog(program, 1024, &log_length, message);
    fprintf(stderr, "Error compiling fragment shader:\n%s\n", message);
    return {Error::ShaderLinking};
  }

  return {program};
}

Result<void> setup_shaders(mem::Arena& arena)
{
  {
    arena.start_temp();

    auto v_shader_res = setup_shader(arena, "src/shader.vert", ShaderType::Vertex);
    if (v_shader_res.has_error)
    {
      return {v_shader_res.err};
    }

    auto f_shader_res = setup_shader(arena, "src/shader.frag", ShaderType::Fragment);
    if (f_shader_res.has_error)
    {
      return {f_shader_res.err};
    }

    arena.stop_temp();

    auto shader_res = link_shaders(v_shader_res.val, f_shader_res.val);
    if (shader_res.has_error)
    {
      return {shader_res.err};
    }

    shader_map[static_cast<i32>(Shader::DefaultShader)] = shader_res.val;
  }

  return {};
}

}
