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
}

void Mesh::draw() const
{
  glBindVertexArray(vao);

  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.len), GL_UNSIGNED_INT, 0);
}

Result<Mesh> Mesh::from_obj(mem::Arena& perm_arena, mem::Arena& temp_arena, const char* path)
{
  auto file_res = platform::read_entire_file(temp_arena, path);
  if (file_res.has_error)
  {
    ASSERT(false, "couldnt read mesh file");
    return {Error::FileReadingError};
  }

  String file{temp_arena, static_cast<char*>(file_res.val)};
  auto lines = file.split(temp_arena, '\n');

  auto vert_count = file.count('v');
  auto idx_count  = file.count('f') * 3;

  Array<Vertex> vertices{perm_arena, vert_count};
  Array<u32> indices{perm_arena, idx_count};

  for (const auto& line : lines)
  {
    switch (line[0])
    {
      case 'v':
      {
        auto parts = line.split(temp_arena, ' ');

        auto v1 = parts[1].parse<f32>();
        if (v1.has_error)
        {
          return {v1.err};
        }
        auto v2 = parts[2].parse<f32>();
        if (v2.has_error)
        {
          return {v2.err};
        }
        auto v3 = parts[3].parse<f32>();
        if (v3.has_error)
        {
          return {v3.err};
        }

        vertices.push({{v1.val, v2.val, v3.val}});
      } break;
      case 'f':
      {
        auto parts = line.split(temp_arena, ' ');
        auto i1 = parts[1].parse<u32>();
        if (i1.has_error)
        {
          return {i1.err};
        }
        auto i2 = parts[2].parse<u32>();
        if (i2.has_error)
        {
          return {i2.err};
        }
        auto i3 = parts[3].parse<u32>();
        if (i3.has_error)
        {
          return {i3.err};
        }

        indices.push(i1.val - 1);
        indices.push(i2.val - 1);
        indices.push(i3.val - 1);
      } break;
    }
  }

  return {{vertices, indices}};
}

void Model::draw(Shader shader) const
{
  glUniformMatrix4fv(glGetUniformLocation(shader_map[static_cast<usize>(shader)], "model"),
                     1, false, model.data);

  for (const auto& mesh : meshes)
  {
    mesh.draw();
  }
}

void Model::rotate(f32 deg, const Vec3& axis)
{
  model.rotate(radians(deg), axis);
}

void Scene::draw() const
{
  for (const auto& drawable : drawables)
  {
    glUseProgram(shader_map[static_cast<usize>(drawable.shader)]);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[static_cast<usize>(drawable.shader)], "view"),
                       1, false, view.data);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[static_cast<usize>(drawable.shader)], "proj"),
                       1, false, proj.data);

    drawable.model.draw(drawable.shader);
  }
}

Result<u32> setup_shader(mem::Arena& arena, const char* filepath,
                                     ShaderType shader_type)
{
  auto shader_src_result = platform::read_entire_file(arena, filepath);
  if (shader_src_result.has_error)
  {
    ASSERT(false, "cannot read shader file");
    return {shader_src_result.err};
  }
  const char* shader_src = static_cast<char*>(shader_src_result.val);

  u32 shader;
  switch (shader_type)
  {
    case ShaderType::Vertex:
    {
      shader = glCreateShader(GL_VERTEX_SHADER);
    } break;

    case ShaderType::Fragment:
    {
      shader = glCreateShader(GL_FRAGMENT_SHADER);
    } break;
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
      case ShaderType::Vertex:
      {
        fprintf(stderr, "Error compiling vertex shader:\n%s\n", message);
        return {Error::ShaderCompilation};
      } break;

      case ShaderType::Fragment:
      {
        fprintf(stderr, "Error compiling fragment shader:\n%s\n", message);
        return {Error::ShaderCompilation};
      } break;
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
