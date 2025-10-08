#include "ogl_renderer.h"

#include "glad/src/glad.c"

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

#include <stdio.h>

Mesh
Mesh::make(Array<Vertex>* vertices, Array<u32>* indices, Material* material)
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
obj_vertex_from_index_part(String* part, Array<Vec3>* vertices, Array<Vec3>* normals,
                           Array<Vec2>* uvs, Arena* temp_arena, Error* err)
{
  Error error = Error::Success;
  Vertex vertex = {};

  Array<String> index = part->split('/', temp_arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, vertex);
  ERROR_ASSERT(index.len == 3, *err, Error::ObjInvalidData, vertex);

  u32 vertex_idx = index[0].parse<u32>(&error);
  ERROR_ASSERT(error == Error::Success, *err, error, vertex);
  vertex.pos = (*vertices)[vertex_idx - 1];

  u32 uv_idx = index[1].parse<u32>(&error);
  ERROR_ASSERT(error == Error::Success, *err, error, vertex);
  vertex.uv = (*uvs)[uv_idx - 1];

  u32 normal_idx = index[2].parse<u32>(&error);
  ERROR_ASSERT(error == Error::Success, *err, error, vertex);
  vertex.normal = (*normals)[normal_idx - 1];

  return vertex;
}

static usize
obj_vertex_get_idx(Array<Vertex>* vertices, Vertex* vertex)
{
  for (usize i = 0; i < vertices->len; ++i)
  {
    if (mem_compare(&(*vertices)[i], vertex, sizeof(Vertex))) return i;
  }
  return (usize) -1;
}

static void
obj_vertex_push_if_missing_and_push_idx(Array<Vertex>* vertices, Array<u32>* indices, Vertex* vertex)
{
  usize vertex_idx = obj_vertex_get_idx(vertices, vertex);
  if (vertex_idx == (usize) -1)
  {
    vertices->push(*vertex);
    vertex_idx = vertices->len - 1;
  }
  indices->push((u32) vertex_idx);
}

#define OBJ_MTL_NEW_HEADER "newmtl "
#define OBJ_MTL_NEW_HEADER_SIZE 7
#define OBJ_MTL_TEXTURE_FILE_HEADER "map_Kd "
#define OBJ_MTL_TEXTURE_FILE_HEADER_SIZE 7

static void
obj_parse_material_file(const char* mtl_file_cstr, usize mtl_file_cstr_length,
                        Arena* temp_arena, Arena* perm_arena, Error* err)
{
  Error error = Error::Success;
  String mtl_file = String::make_cstr_len(mtl_file_cstr, mtl_file_cstr_length, temp_arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error,);
  Array<String> lines = mtl_file.split('\n', temp_arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error,);

  b32 parsing_flag = false;
  Material mat = {};
  for (usize line_idx = 0; line_idx < lines.len; ++line_idx)
  {
    if (mem_compare(lines[line_idx].data, OBJ_MTL_NEW_HEADER, OBJ_MTL_NEW_HEADER_SIZE))
    {
      parsing_flag = true;
    }
    else if (mem_compare(lines[line_idx].data,
                         OBJ_MTL_TEXTURE_FILE_HEADER, OBJ_MTL_TEXTURE_FILE_HEADER_SIZE))
    {
      if (!parsing_flag) continue;

      Array<String> parts = lines[line_idx].split(' ', temp_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error,);
      String texture_file_path = parts[1].prepend("assets/", temp_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error,);
      // TODO(szulf): probably need some asset manager to not lose this image variable
      // (data is allocated in the arena anyway)
      Image img = image_decode_png(texture_file_path.data, temp_arena, perm_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error,);
      mat.texture = Texture::make(&img);
      g_materials.push(mat);
    }
  }
}

// TODO(szulf): shouldnt this return a model and not a mesh?
Mesh
Mesh::from_obj(const char* path, Arena* temp_arena, Arena* perm_arena, Error* err)
{
  Error error = Error::Success;

  usize data_len;
  const char* data = (const char*) platform_read_entire_file(path, temp_arena, &error, &data_len);
  ERROR_ASSERT(error == Error::Success, *err, error, {});
  String file = String::make_cstr_len(data, data_len, temp_arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, {});
  Array<String> lines = file.split('\n', temp_arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, {});

  usize vert_count = file.count_substrings(OBJ_VERTEX_HEADER);
  usize normal_count = file.count_substrings(OBJ_NORMAL_HEADER);
  usize uv_count = file.count_substrings(OBJ_UV_HEADER);
  usize index_count = file.count_substrings(OBJ_INDEX_HEADER) * 3;
  UNUSED(index_count);

  Array<Vec3> positions = Array<Vec3>::make(vert_count, temp_arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, {});

  Array<Vec3> normals = Array<Vec3>::make(normal_count, temp_arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, {});

  Array<Vec2> uvs = Array<Vec2>::make(uv_count, temp_arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, {});

  // TODO(szulf): change this size to something else than a 10000
  Array<Vertex> vertices = Array<Vertex>::make(10000, perm_arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, {});
  Array<u32> indices = Array<u32>::make(10000, perm_arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, {});

  for (usize line_idx = 0; line_idx < lines.len; ++line_idx)
  {
    if (mem_compare(lines[line_idx].data,
                    OBJ_MATERIAL_FILE_HEADER, OBJ_MATERIAL_FILE_HEADER_SIZE))
    {
      Array<String> splits = lines[line_idx].split(' ', temp_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      String material_file_path = splits[1].prepend("assets/", temp_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      usize material_file_size;
      void* material_file = platform_read_entire_file(material_file_path.data, temp_arena, &error,
                                                      &material_file_size);
      obj_parse_material_file((const char*) material_file, material_file_size,
                              temp_arena, perm_arena, &error);
    }
    else if (mem_compare(lines[line_idx].data, OBJ_VERTEX_HEADER, OBJ_VERTEX_HEADER_SIZE))
    {
      Array<String> splits = lines[line_idx].split(' ', temp_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      ERROR_ASSERT(splits.len == 4, *err, Error::ObjInvalidData, {});

      f32 v1 = splits[1].parse<f32>(&error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      f32 v2 = splits[2].parse<f32>(&error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      f32 v3 = splits[3].parse<f32>(&error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});

      positions.push({v1, v2, v3});
    }
    else if (mem_compare(lines[line_idx].data, OBJ_NORMAL_HEADER, OBJ_NORMAL_HEADER_SIZE))
    {
      Array<String> splits = lines[line_idx].split(' ', temp_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      ERROR_ASSERT(splits.len == 4, *err, Error::ObjInvalidData, {});

      f32 n1 = splits[1].parse<f32>(&error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      f32 n2 = splits[2].parse<f32>(&error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      f32 n3 = splits[3].parse<f32>(&error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});

      normals.push({n1, n2, n3});
    }
    else if (mem_compare(lines[line_idx].data,
                         OBJ_UV_HEADER, OBJ_UV_HEADER_SIZE))
    {
      Array<String> splits = lines[line_idx].split(' ', temp_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      ERROR_ASSERT(splits.len == 3, *err, Error::ObjInvalidData, {});

      f32 u = splits[1].parse<f32>(&error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      f32 v = splits[2].parse<f32>(&error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});

      uvs.push({u, v});
    }
    else if (mem_compare(lines[line_idx].data, OBJ_INDEX_HEADER, OBJ_INDEX_HEADER_SIZE))
    {
      Array<String> parts = lines[line_idx].split(' ', temp_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      ERROR_ASSERT(parts.len == 4, *err, Error::ObjInvalidData, {});

      Vertex vertex_1 = obj_vertex_from_index_part(&parts[1], &positions, &normals, &uvs,
                                                   temp_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      obj_vertex_push_if_missing_and_push_idx(&vertices, &indices, &vertex_1);

      Vertex vertex_2 = obj_vertex_from_index_part(&parts[2], &positions, &normals, &uvs,
                                                   temp_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      obj_vertex_push_if_missing_and_push_idx(&vertices, &indices, &vertex_2);

      Vertex vertex_3 = obj_vertex_from_index_part(&parts[3], &positions, &normals, &uvs,
                                                   temp_arena, &error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      obj_vertex_push_if_missing_and_push_idx(&vertices, &indices, &vertex_3);
    }
  }

  return Mesh::make(&vertices, &indices, &g_materials[0]);
}

void
Model::draw(Shader shader) const
{
  glUniformMatrix4fv(glGetUniformLocation(shader_map[(usize) shader], "model"),
                     1, false, model.data);

  for (usize mesh_idx = 0; mesh_idx < meshes.len; ++mesh_idx)
  {
    meshes[mesh_idx].draw(shader);
  }
}

void
Model::rotate(f32 deg, const Vec3* axis)
{
  model.rotate(radians(deg), axis);
}

void
Scene::draw() const
{
  for (usize drawable_idx = 0; drawable_idx < drawables.len; ++drawable_idx)
  {
    const Drawable* drawable = &drawables[drawable_idx];

    glUseProgram(shader_map[(usize) drawable->shader]);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[(usize) drawable->shader], "view"),
                       1, false, view.data);
    glUniformMatrix4fv(glGetUniformLocation(shader_map[(usize) drawable->shader], "proj"),
                       1, false, proj.data);

    drawables[drawable_idx].model.draw(drawables[drawable_idx].shader);
  }
}

static u32
setup_shader(const char* path, ShaderType shader_type, Arena* arena, Error* err)
{
  Error error = Error::Success;
  const char* shader_src = (const char*) platform_read_entire_file(path, arena, &error);
  ASSERT(error == Error::Success, "cannot read shader file");

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
      case ShaderType::Vertex:
      {
        LOG("Error compiling vertex shader:\n%s\n", message);
        *err = Error::ShaderCompilation;
        return (u32) -1;
      } break;
      case ShaderType::Fragment:
      {
        LOG("Error compiling fragment shader:\n%s\n", message);
        *err = Error::ShaderCompilation;
        return (u32) -1;
      } break;
    }
  }

  *err = Error::Success;
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
    *err = Error::ShaderLinking;
    return (u32) -1;
  }

  *err = Error::Success;
  return program;
}

static void
setup_shaders(Arena* arena, Error* err)
{
  Error error = Error::Success;

  {
    u32 v_shader = setup_shader("src/shader.vert", ShaderType::Vertex, arena, &error);
    ASSERT(error == Error::Success, "couldnt setup vertex shader");

    u32 f_shader = setup_shader("src/shader.frag", ShaderType::Fragment, arena, &error);
    ASSERT(error == Error::Success, "couldnt setup fragment shader");

    u32 shader = link_shaders(v_shader, f_shader, &error);
    ASSERT(error == Error::Success, "couldnt link shader");

    shader_map[(usize) Shader::Default] = shader;
  }

  *err = Error::Success;
}

Texture
Texture::make(Image* img)
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
