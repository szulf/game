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

// TODO(szulf): i dont think these asserts should happen at this level
// TODO(szulf): shouldnt this return a model and not a mesh?
static Mesh
mesh_from_obj(Arena* perm_arena, Arena* temp_arena, const char* path, Error* err)
{
  Mesh mesh = {};
  Error error = ERROR_SUCCESS;
  char* file_content = platform_read_entire_file(temp_arena, path, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt read mesh file");

  String file = string_make_cstr(temp_arena, file_content, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt init string");

  StringArray lines = string_split(&file, temp_arena, '\n', &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt split file into lines");

  usize vert_count = string_count_chars(&file, 'v');
  usize idx_count = string_count_chars(&file, 'f') * 3;

  VertexArray vertices = {};
  ARRAY_INIT(&vertices, perm_arena, vert_count, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt init array");
  U32Array indices = {};
  ARRAY_INIT(&indices, perm_arena, idx_count, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt init array");

  for (usize line_idx = 0; line_idx < lines.len; ++line_idx)
  {
    switch (lines.items[line_idx].data[0])
    {
      case 'v':
      {
        StringArray parts = string_split(&lines.items[line_idx], temp_arena, ' ', &error);
        ASSERT(error == ERROR_SUCCESS, "couldnt split string");
        ASSERT(parts.len >= 4, "invalid obj file");

        f32 v1 = string_parse_f32(&parts.items[1], &error);
        ASSERT(error == ERROR_SUCCESS, "couldnt parse vertex");
        f32 v2 = string_parse_f32(&parts.items[2], &error);
        ASSERT(error == ERROR_SUCCESS, "couldnt parse vertex");
        f32 v3 = string_parse_f32(&parts.items[3], &error);
        ASSERT(error == ERROR_SUCCESS, "couldnt parse vertex");

        ARRAY_PUSH(&vertices, ((Vertex) {{v1, v2, v3}}));
      } break;
      case 'f':
      {
        StringArray parts = string_split(&lines.items[line_idx], temp_arena, ' ', &error);
        ASSERT(error == ERROR_SUCCESS, "couldnt split string");
        ASSERT(parts.len >= 4, "invalid obj file");

        u32 i1 = string_parse_u32(&parts.items[1], &error);
        ASSERT(error == ERROR_SUCCESS, "couldnt parse index");
        u32 i2 = string_parse_u32(&parts.items[2], &error);
        ASSERT(error == ERROR_SUCCESS, "couldnt parse index");
        u32 i3 = string_parse_u32(&parts.items[3], &error);
        ASSERT(error == ERROR_SUCCESS, "couldnt parse index");

        ARRAY_PUSH(&indices, i1 - 1);
        ARRAY_PUSH(&indices, i2 - 1);
        ARRAY_PUSH(&indices, i3 - 1);
      } break;
    }
  }

  mesh_init(&mesh, &vertices, &indices);
  *err = ERROR_SUCCESS;
  return mesh;
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

static u32
setup_shader(Arena* arena, const char* path, ShaderType shader_type, Error* err)
{
  Error error = ERROR_SUCCESS;
  const char* shader_src = platform_read_entire_file(arena, path, &error);
  ASSERT(error == ERROR_SUCCESS, "cannot read shader file");

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
        LOG("Error compiling vertex shader:\n%s\n", message);
        *err = ERROR_SHADER_COMPILATION;
        return (u32) -1;
      } break;
      case SHADER_TYPE_FRAGMENT:
      {
        LOG("Error compiling fragment shader:\n%s\n", message);
        *err = ERROR_SHADER_COMPILATION;
        return (u32) -1;
      } break;
    }
  }

  *err = ERROR_SUCCESS;
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
    LOG("Error compiling fragment shader:\n%s\n", message);
    *err = ERROR_SHADER_LINKING;
    return (u32) -1;
  }

  *err = ERROR_SUCCESS;
  return program;
}

static void
setup_shaders(Arena* arena, Error* err)
{
  Error error = ERROR_SUCCESS;

  {
    u32 v_shader = setup_shader(arena, "src/shader.vert", SHADER_TYPE_VERTEX, &error);
    ASSERT(error == ERROR_SUCCESS, "couldnt setup vertex shader");

    u32 f_shader = setup_shader(arena, "src/shader.frag", SHADER_TYPE_FRAGMENT, &error);
    ASSERT(error == ERROR_SUCCESS, "couldnt setup fragment shader");

    u32 shader = link_shaders(v_shader, f_shader, &error);
    ASSERT(error == ERROR_SUCCESS, "couldnt link shader");

    shader_map[SHADER_DEFAULT] = shader;
  }

  *err = ERROR_SUCCESS;
}
