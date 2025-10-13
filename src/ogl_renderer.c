#include "ogl_renderer.h"

#ifdef GAME_SDL
#include "ogl_functions.h"
#endif

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
mesh_make(VertexArray* vertices, U32Array* indices, Material* material)
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

#define OBJ_VERTEX_HEADER "v "
#define OBJ_VERTEX_HEADER_SIZE 2
#define OBJ_NORMAL_HEADER "vn "
#define OBJ_NORMAL_HEADER_SIZE 3
#define OBJ_UV_HEADER "vt "
#define OBJ_UV_HEADER_SIZE 3
#define OBJ_INDEX_HEADER "f "
#define OBJ_INDEX_HEADER_SIZE 2
#define OBJ_MATERIAL_FILE_HEADER "mtllib "
#define OBJ_MATERIAL_FILE_HEADER_SIZE 7

static Vertex
obj_vertex_from_index_part(String* part, Vec3Array* vertices, Vec3Array* normals, Vec2Array* uvs,
                Arena* temp_arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  Vertex vertex = {0};

  StringArray index = string_split(part, '/', temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, vertex);
  ERROR_ASSERT(index.len == 3, *err, ERROR_OBJ_INVALID_DATA, vertex);

  u32 vertex_idx = string_parse_u32(&index.items[0], &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, vertex);
  vertex.pos = vertices->items[vertex_idx - 1];

  u32 uv_idx = string_parse_u32(&index.items[1], &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, vertex);
  vertex.uv = uvs->items[uv_idx - 1];

  u32 normal_idx = string_parse_u32(&index.items[2], &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, vertex);
  vertex.normal = normals->items[normal_idx - 1];

  return vertex;
}

static usize
obj_vertex_get_idx(VertexArray* vertices, Vertex* vertex)
{
  for (usize i = 0; i < vertices->len; ++i)
  {
    if (mem_compare(&vertices->items[i], vertex, sizeof(Vertex))) return i;
  }
  return (usize) -1;
}

static void
obj_vertex_push_if_missing_and_push_idx(VertexArray* vertices, U32Array* indices, Vertex* vertex)
{
  usize vertex_idx = obj_vertex_get_idx(vertices, vertex);
  if (vertex_idx == (usize) -1)
  {
    ARRAY_PUSH(vertices, *vertex);
    vertex_idx = vertices->len - 1;
  }
  ARRAY_PUSH(indices, ((u32) vertex_idx));
}

#define OBJ_MTL_NEW_HEADER "newmtl "
#define OBJ_MTL_NEW_HEADER_SIZE 7
#define OBJ_MTL_TEXTURE_FILE_HEADER "map_Kd "
#define OBJ_MTL_TEXTURE_FILE_HEADER_SIZE 7

static void
obj_parse_material_file(const char* mtl_file_cstr, usize mtl_file_cstr_length,
                        Arena* temp_arena, Arena* perm_arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  String mtl_file = string_make_cstr_len(mtl_file_cstr, mtl_file_cstr_length);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
  StringArray lines = string_split(&mtl_file, '\n', temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);

  b32 parsing_flag = false;
  Material mat = {0};
  for (usize line_idx = 0; line_idx < lines.len; ++line_idx)
  {
    if (mem_compare(lines.items[line_idx].data, OBJ_MTL_NEW_HEADER, OBJ_MTL_NEW_HEADER_SIZE))
    {
      parsing_flag = true;
    }
    else if (mem_compare(lines.items[line_idx].data, OBJ_MTL_TEXTURE_FILE_HEADER, OBJ_MTL_TEXTURE_FILE_HEADER_SIZE))
    {
      if (!parsing_flag) continue;

      StringArray parts = string_split(&lines.items[line_idx], ' ', temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
      String texture_file_path = string_prepend(&parts.items[1], "assets/", temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
      usize img_file_size;
      void* img_file = os_read_entire_file_bytes_read(texture_file_path.data, &img_file_size,
                                                            temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);

      // TODO(szulf): probably need some asset manager to not lose this image variable
      // (data is allocated in the arena anyway)
      Image img = image_decode_png(img_file, img_file_size, temp_arena, perm_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
      mat.texture = texture_make(&img);
      ARRAY_PUSH(&g_materials, mat);
    }
  }
}

// TODO(szulf): shouldnt this return a model and not a mesh?
static Mesh
mesh_from_obj(void* data, usize data_len, Arena* temp_arena, Arena* perm_arena, Error* err)
{
  Error error = ERROR_SUCCESS;

  String file = string_make_cstr_len(data, data_len);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
  StringArray lines = string_split(&file, '\n', temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});

  usize vert_count = string_count_substrings(&file, OBJ_VERTEX_HEADER);
  usize normal_count = string_count_substrings(&file, OBJ_NORMAL_HEADER);
  usize uv_count = string_count_substrings(&file, OBJ_UV_HEADER);
  usize index_count = string_count_substrings(&file, OBJ_INDEX_HEADER) * 3;

  Vec3Array positions = {0};
  ARRAY_INIT(&positions, vert_count, temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
  U32Array position_indices = {0};
  ARRAY_INIT(&position_indices, index_count, temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});

  Vec3Array normals = {0};
  ARRAY_INIT(&normals, normal_count, temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
  U32Array normal_indices = {0};
  ARRAY_INIT(&normal_indices, index_count, temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});

  Vec2Array uvs = {0};
  ARRAY_INIT(&uvs, uv_count, temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
  U32Array uv_indices = {0};
  ARRAY_INIT(&uv_indices, index_count, temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});

  VertexArray vertices = {0};
  ARRAY_INIT(&vertices, index_count, perm_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
  U32Array indices = {0};
  ARRAY_INIT(&indices, index_count, perm_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});

  for (usize line_idx = 0; line_idx < lines.len; ++line_idx)
  {
    if (mem_compare(lines.items[line_idx].data,
                    OBJ_MATERIAL_FILE_HEADER, OBJ_MATERIAL_FILE_HEADER_SIZE))
    {
      StringArray splits = string_split(&lines.items[line_idx], ' ', temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      String material_file_path = string_prepend(&splits.items[1], "assets/", temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      usize material_file_size;
      void* material_file =
        os_read_entire_file_bytes_read(material_file_path.data, &material_file_size,
                                             temp_arena, &error);
      obj_parse_material_file(material_file, material_file_size, temp_arena, perm_arena, &error);
    }
    else if (mem_compare(lines.items[line_idx].data, OBJ_VERTEX_HEADER, OBJ_VERTEX_HEADER_SIZE))
    {
      StringArray splits = string_split(&lines.items[line_idx], ' ', temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      ERROR_ASSERT(splits.len == 4, *err, ERROR_OBJ_INVALID_DATA, (Mesh) {0});

      f32 v1 = string_parse_f32(&splits.items[1], &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      f32 v2 = string_parse_f32(&splits.items[2], &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      f32 v3 = string_parse_f32(&splits.items[3], &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});

      ARRAY_PUSH(&positions, ((Vec3) {v1, v2, v3}));
    }
    else if (mem_compare(lines.items[line_idx].data, OBJ_NORMAL_HEADER, OBJ_NORMAL_HEADER_SIZE))
    {
      StringArray splits = string_split(&lines.items[line_idx], ' ', temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      ERROR_ASSERT(splits.len == 4, *err, ERROR_OBJ_INVALID_DATA, (Mesh) {0});

      f32 n1 = string_parse_f32(&splits.items[1], &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      f32 n2 = string_parse_f32(&splits.items[2], &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      f32 n3 = string_parse_f32(&splits.items[3], &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});

      ARRAY_PUSH(&normals, ((Vec3) {n1, n2, n3}));
    }
    else if (mem_compare(lines.items[line_idx].data,
                         OBJ_UV_HEADER, OBJ_UV_HEADER_SIZE))
    {
      StringArray splits = string_split(&lines.items[line_idx], ' ', temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      ERROR_ASSERT(splits.len == 3, *err, ERROR_OBJ_INVALID_DATA, (Mesh) {0});

      f32 u = string_parse_f32(&splits.items[1], &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      f32 v = string_parse_f32(&splits.items[2], &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});

      ARRAY_PUSH(&uvs, ((Vec2) {u, v}));
    }
    else if (mem_compare(lines.items[line_idx].data, OBJ_INDEX_HEADER, OBJ_INDEX_HEADER_SIZE))
    {
      StringArray parts = string_split(&lines.items[line_idx], ' ', temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      ERROR_ASSERT(parts.len == 4, *err, ERROR_OBJ_INVALID_DATA, (Mesh) {0});

      Vertex vertex_1 = obj_vertex_from_index_part(&parts.items[1], &positions, &normals, &uvs,
                                                   temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      obj_vertex_push_if_missing_and_push_idx(&vertices, &indices, &vertex_1);

      Vertex vertex_2 = obj_vertex_from_index_part(&parts.items[2], &positions, &normals, &uvs,
                                                   temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      obj_vertex_push_if_missing_and_push_idx(&vertices, &indices, &vertex_2);

      Vertex vertex_3 = obj_vertex_from_index_part(&parts.items[3], &positions, &normals, &uvs,
                                                   temp_arena, &error);
      ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, (Mesh) {0});
      obj_vertex_push_if_missing_and_push_idx(&vertices, &indices, &vertex_3);
    }
  }

  return mesh_make(&vertices, &indices, &g_materials.items[0]);
}

static void
model_draw(const Model* model, Shader shader)
{
  glUniformMatrix4fv(glGetUniformLocation(shader_map[shader], "model"),
                     1, false, model->model.data);

  for (usize mesh_idx = 0; mesh_idx < model->meshes.len; ++mesh_idx)
  {
    mesh_draw(&model->meshes.items[mesh_idx], shader);
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
    Renderable* renderable = &scene->renderables.items[renderable_idx];

    glUseProgram(shader_map[renderable->shader]);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[renderable->shader], "view"),
                       1, false, scene->view.data);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[renderable->shader], "proj"),
                       1, false, scene->proj.data);

    model_draw(&scene->renderables.items[renderable_idx].model,
               scene->renderables.items[renderable_idx].shader);
  }
}

static u32
setup_shader(Arena* arena, const char* path, ShaderType shader_type, Error* err)
{
  Error error = ERROR_SUCCESS;
  const char* shader_src = os_read_entire_file(path, arena, &error);
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
