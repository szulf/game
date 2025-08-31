#include "ogl_renderer.h"

#include "glad/src/glad.c"

static void
clear_screen()
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

static void
mesh_init(Mesh* mesh, const VertexArray* vertices, const U32Array* indices)
{
  mesh->vertices = *vertices;
  mesh->indices = *indices;
  
  glGenVertexArrays(1, &mesh->vao);
  glBindVertexArray(mesh->vao);

  glGenBuffers(1, &mesh->vbo);
  glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
  glBufferData(GL_ARRAY_BUFFER, (GLsizei) (mesh->vertices.len * sizeof(Vertex)),
               mesh->vertices.items, GL_STATIC_DRAW);

  glGenBuffers(1, &mesh->ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei) (mesh->indices.len * sizeof(u32)),
               mesh->indices.items, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (void*) 0);
}

static void
mesh_draw(const Mesh* mesh)
{
  glBindVertexArray(mesh->vao);

  glDrawElements(GL_TRIANGLES, (GLsizei) mesh->indices.len, GL_UNSIGNED_INT, 0);
}

static Error
mesh_from_obj(Mesh* mesh, Arena* perm_arena, Arena* temp_arena, const char* path)
{
  char* file_content = {};
  Error file_error = platform_read_entire_file((void**) &file_content, temp_arena, path);
  if (file_error != SUCCESS)
  {
    ASSERT(false, "couldnt read mesh file");
    return file_error;
  }

  String file = {};
  string_init_cstr(&file, temp_arena, (char*) file_content);

  StringArray lines = {};
  Error splitting_err = string_split(&lines, &file, temp_arena, '\n');
  if (splitting_err != SUCCESS)
  {
    ASSERT(false, "couldnt split file into lines");
    return splitting_err;
  }

  usize vert_count = string_count_chars(&file, 'v');
  usize idx_count = string_count_chars(&file, 'f') * 3;

  VertexArray vertices = {};
  ARRAY_INIT(&vertices, perm_arena, vert_count);
  U32Array indices = {};
  ARRAY_INIT(&indices, perm_arena, idx_count);

  for (usize line_idx = 0; line_idx < lines.len; ++line_idx)
  {
    switch (lines.items[line_idx].data[0])
    {
      case 'v':
      {
        StringArray parts = {};
        splitting_err = string_split(&parts, &lines.items[line_idx], temp_arena, ' ');
        if (splitting_err != SUCCESS)
        {
          return splitting_err;
        }

        if (parts.len < 4)
        {
          return INVALID_PARAMETER;
        }
        
        f32 v1 = 0.0f;
        Error parsing_err = string_parse_f32(&v1, &parts.items[1]);
        if (parsing_err != SUCCESS)
        {
          return parsing_err;
        }
        f32 v2 = 0.0f;
        parsing_err = string_parse_f32(&v2, &parts.items[2]);
        if (parsing_err != SUCCESS)
        {
          return parsing_err;
        }
        f32 v3 = 0.0f;
        parsing_err = string_parse_f32(&v3, &parts.items[3]);
        if (parsing_err != SUCCESS)
        {
          return parsing_err;
        }

        ARRAY_PUSH(&vertices, ((Vertex) {{v1, v2, v3}}));
      } break;
      case 'f':
      {
        StringArray parts = {};
        splitting_err = string_split(&parts, &lines.items[line_idx], temp_arena, ' ');
        if (splitting_err != SUCCESS)
        {
          return splitting_err;
        }

        if (parts.len < 4)
        {
          return INVALID_PARAMETER;
        }
        
        u32 i1 = 0;
        Error parsing_error;
        parsing_error = string_parse_u32(&i1, &parts.items[1]);
        if (parsing_error != SUCCESS)
        {
          return parsing_error;
        }
        u32 i2 = 0;
        parsing_error = string_parse_u32(&i2, &parts.items[2]);
        if (parsing_error != SUCCESS)
        {
          return parsing_error;
        }
        u32 i3 = 0;
        parsing_error = string_parse_u32(&i3, &parts.items[3]);
        if (parsing_error != SUCCESS)
        {
          return parsing_error;
        }

        ARRAY_PUSH(&indices, i1 - 1);
        ARRAY_PUSH(&indices, i2 - 1);
        ARRAY_PUSH(&indices, i3 - 1);
      } break;
    }
  }

  mesh_init(mesh, &vertices, &indices);
  return SUCCESS;
}

static void
model_draw(const Model* model, Shader shader)
{
  glUniformMatrix4fv(glGetUniformLocation(shader_map[shader], "model"),
                     1, false, model->model.data);

  for (usize mesh_idx = 0; mesh_idx < model->meshes.len; ++mesh_idx)
  {
    mesh_draw(&model->meshes.items[mesh_idx]);
  }
}

static void
model_rotate(Model* model, f32 deg, const Vec3* axis)
{
  mat4_rotate(&model->model, radians(deg), axis);
}

static void
scene_draw(const Scene* scene)
{
  for (usize drawable_idx = 0; drawable_idx < scene->drawables.len; ++drawable_idx)
  {
    Drawable* drawable = &scene->drawables.items[drawable_idx];

    glUseProgram(shader_map[drawable->shader]);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[drawable->shader], "view"),
                       1, false, scene->view.data);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[drawable->shader], "proj"),
                       1, false, scene->proj.data);

    model_draw(&scene->drawables.items[drawable_idx].model, scene->drawables.items[drawable_idx].shader);
  }
}

static Error
setup_shader(u32* out, Arena* arena, const char* path, ShaderType shader_type)
{
  const char* shader_src;
  Error file_reading_error = platform_read_entire_file((void**) &shader_src, arena, path);
  if (file_reading_error != SUCCESS)
  {
    ASSERT(false, "cannot read shader file");
    return file_reading_error;
  }

  u32 shader;
  switch (shader_type)
  {
    case SHADER_TYPE_VERTEX:
    {
      shader = glCreateShader(GL_VERTEX_SHADER);
    } break;

    case SHADER_TYPE_FRAGMENT:
    {
      shader = glCreateShader(GL_FRAGMENT_SHADER);
    } break;
  }

  glShaderSource(shader, 1, &shader_src, 0);
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
      case SHADER_TYPE_VERTEX:
      {
        // TODO(szulf): change this to my logging
        fprintf(stderr, "Error compiling vertex shader:\n%s\n", message);
        return SHADER_COMPILATION;
      } break;
      case SHADER_TYPE_FRAGMENT:
      {
        // TODO(szulf): change this to my logging
        fprintf(stderr, "Error compiling fragment shader:\n%s\n", message);
        return SHADER_COMPILATION;
      } break;
    }
  }

  *out = shader;
  return SUCCESS;
}

static Error
link_shaders(u32* out, u32 vertex_shader, u32 fragment_shader)
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
    // TODO(szulf): change this to my logging
    fprintf(stderr, "Error compiling fragment shader:\n%s\n", message);
    return SHADER_LINKING;
  }

  *out = program;
  return SUCCESS;
}

static Error
setup_shaders(Arena* arena)
{
  {
    u32 v_shader;
    Error v_shader_err = setup_shader(&v_shader, arena, "src/shader.vert", SHADER_TYPE_VERTEX);
    if (v_shader_err != SUCCESS)
    {
      return v_shader_err;
    }

    u32 f_shader;
    Error f_shader_err = setup_shader(&f_shader, arena, "src/shader.frag", SHADER_TYPE_FRAGMENT);
    if (f_shader_err != SUCCESS)
    {
      return f_shader_err;
    }

    u32 shader;
    Error shader_err = link_shaders(&shader, v_shader, f_shader);
    if (shader_err != SUCCESS)
    {
      return shader_err;
    }

    shader_map[SHADER_DEFAULT] = shader;
  }

  return SUCCESS;
}
