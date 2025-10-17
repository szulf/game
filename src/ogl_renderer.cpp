#include "ogl_renderer.h"

#include "ogl_functions.h"

static void
setup_renderer(void)
{
  glEnable(GL_DEPTH_TEST);
}

static void
clear_screen(void)
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static Mesh
mesh_make(Array<Vertex>* vertices, Array<u32>* indices, Material* material)
{
  Mesh mesh;
  mesh.vertices = *vertices;
  mesh.indices = *indices;
  mesh.material = *material;

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

static void
mesh_draw(const Mesh* mesh, Shader shader)
{
  // TODO(szulf): this should not happen for every mesh,
  // should also check how many textures should be made active
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mesh->material.texture.id);
  glUniform1i(glGetUniformLocation(shader_map[shader], "sampler"), 0);
  glBindVertexArray(mesh->vao);
  glDrawElements(GL_TRIANGLES, (GLsizei) mesh->indices.len, GL_UNSIGNED_INT, 0);
}

static void
model_draw(const Model* model, Shader shader)
{
  glUniformMatrix4fv(glGetUniformLocation(shader_map[shader], "model"),
                     1, false, model->model.data);

  for (usize mesh_idx = 0; mesh_idx < model->meshes.len; ++mesh_idx)
  {
    mesh_draw(&model->meshes[mesh_idx], shader);
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
  for (usize renderable_idx = 0; renderable_idx < scene->renderables.len; ++renderable_idx)
  {
    const Renderable* renderable = &scene->renderables[renderable_idx];

    glUseProgram(shader_map[renderable->shader]);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[renderable->shader], "view"),
                       1, false, scene->view.data);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[renderable->shader], "proj"),
                       1, false, scene->proj.data);

    model_draw(&scene->renderables[renderable_idx].model,
               scene->renderables[renderable_idx].shader);
  }
}

static u32
setup_shader(Arena* arena, const char* path, ShaderType shader_type, Error* err)
{
  Error error = ERROR_SUCCESS;
  const char* shader_src = (const char*) os_read_entire_file(path, arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (u32) -1);

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
        LOG("Error compiling vertex shader:\n{}", message);
        *err = ERROR_SHADER_COMPILATION;
        return (u32) -1;
      } break;
      case SHADER_TYPE_FRAGMENT:
      {
        LOG("Error compiling fragment shader:\n{}", message);
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
    LOG("Error compiling fragment shader:\n{}", message);
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
    ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);

    u32 f_shader = setup_shader(arena, "src/shader.frag", SHADER_TYPE_FRAGMENT, &error);
    ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);

    u32 shader = link_shaders(v_shader, f_shader, &error);
    ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);

    shader_map[SHADER_DEFAULT] = shader;
  }

  *err = ERROR_SUCCESS;
}

static Texture
texture_make(Image* img)
{
  Texture texture = {0};

  glGenTextures(1, &texture.id);
  glBindTexture(GL_TEXTURE_2D, texture.id);

  // TODO(szulf): do i want to customize these
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei) img->width, (GLsizei) img->height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, img->data);
  glGenerateMipmap(GL_TEXTURE_2D);

  return texture;
}
