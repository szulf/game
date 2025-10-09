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

Mesh::Mesh(const std::pmr::vector<Vertex>& vertices, const std::pmr::vector<u32>& indices,
           const Material& material) : vertices{vertices}, indices{indices}, material{material}
{
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  u32 vertex_vbo;
  u32 vertex_ebo;
  glGenBuffers(1, &vertex_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(vertices.size() * sizeof(Vertex)),
               vertices.data(), GL_STATIC_DRAW);
  glGenBuffers(1, &vertex_ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizei>(indices.size() * sizeof(u32)),
               indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void*>(3 * sizeof(f32)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void*>(6 * sizeof(f32)));
  glEnableVertexAttribArray(2);
}

void
Mesh::draw(Shader shader) const
{
  // TODO(szulf): this should not happen for every mesh,
  // should also check how many textures should be made active
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, material.texture.id);
  glUniform1i(glGetUniformLocation(shader_map[static_cast<usize>(shader)], "sampler"), 0);
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
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
obj_vertex_from_index_part(const std::pmr::string& part, const std::pmr::vector<Vec3>& vertices,
                           const std::pmr::vector<Vec3>& normals, const std::pmr::vector<Vec2>& uvs,
                           Error* err)
{
  Vertex vertex{};
  std::size_t pos{0};

  std::size_t slash_idx{part.find('/', pos)};
  ERROR_ASSERT(slash_idx != std::string::npos, *err, Error::ObjInvalidData, {});
  std::size_t vertex_idx{};
  auto error{std::from_chars(part.data() + pos, part.data() + slash_idx, vertex_idx).ec};
  ERROR_ASSERT(error == std::errc(), *err, Error::ObjInvalidData, {});
  vertex.pos = vertices[vertex_idx - 1];
  pos = slash_idx + 1;

  slash_idx = part.find('/', pos);
  ERROR_ASSERT(slash_idx != std::string::npos, *err, Error::ObjInvalidData, {});
  std::size_t uv_idx{};
  error = std::from_chars(part.data() + pos, part.data() + slash_idx, uv_idx).ec;
  ERROR_ASSERT(error == std::errc(), *err, Error::ObjInvalidData, {});
  vertex.uv = uvs[uv_idx - 1];
  pos = slash_idx + 1;

  std::size_t normal_idx{};
  error = std::from_chars(part.data() + pos, part.data() + slash_idx, normal_idx).ec;
  ERROR_ASSERT(error == std::errc(), *err, Error::ObjInvalidData, {});
  vertex.normal = normals[normal_idx - 1];

  return vertex;
}

static usize
obj_vertex_get_idx(const std::pmr::vector<Vertex>& vertices, const Vertex& vertex)
{
  for (usize i = 0; i < vertices.size(); ++i)
  {
    if (vertices[i] == vertex) return i;
  }
  return (usize) -1;
}

static void
obj_vertex_push_if_missing_and_push_idx(std::pmr::vector<Vertex>& vertices,
                                        std::pmr::vector<u32>& indices, const Vertex& vertex)
{
  usize vertex_idx = obj_vertex_get_idx(vertices, vertex);
  if (vertex_idx == static_cast<usize>(-1))
  {
    vertices.push_back(vertex);
    vertex_idx = vertices.size() - 1;
  }
  indices.push_back(static_cast<u32>(vertex_idx));
}

#define OBJ_MTL_NEW_HEADER "newmtl "
#define OBJ_MTL_NEW_HEADER_SIZE 7
#define OBJ_MTL_TEXTURE_FILE_HEADER "map_Kd "
#define OBJ_MTL_TEXTURE_FILE_HEADER_SIZE 7

static void
obj_parse_material_file(const char* mtl_file_cstr, usize mtl_file_cstr_length, Error* err)
{
  Error error{Error::Success};
  // TODO(szulf): does this call the right constructor?
  std::pmr::string mtl_file{mtl_file_cstr, mtl_file_cstr_length};
  std::stringstream mtl_stream{};
  mtl_stream.str(mtl_file);
  std::pmr::string line{};
  bool parsing_flag{false};
  Material mat{};

  while (std::getline(mtl_stream, line))
  {
    if (line.starts_with(OBJ_MTL_NEW_HEADER))
    {
      parsing_flag = true;
    }
    else if (line.starts_with(OBJ_MTL_TEXTURE_FILE_HEADER))
    {
      if (!parsing_flag) continue;

      usize space_idx{line.find(' ')};
      std::pmr::string texture_file_path{line.substr(space_idx + 1)};
      texture_file_path.insert(0, "assets/");
      // TODO(szulf): probably need some asset manager to not lose this image variable
      // (data is allocated in the arena anyway)
      image::Image img = image::decode_png(texture_file_path.data(), &error);
      ERROR_ASSERT(error == Error::Success, *err, error,);
      mat.texture = Texture{img};
      g_materials.push_back(mat);
    }
  }
}

// TODO(szulf): shouldnt this return a model and not a mesh?
Mesh
Mesh::from_obj(const char* path, Error* err)
{
  Error error = Error::Success;

  usize data_len;
  const char* data = (const char*) platform::read_entire_file(path, &error, &data_len);
  ERROR_ASSERT(error == Error::Success, *err, error, {});
  std::pmr::string file{data, data_len};

  std::stringstream file_stream{};
  file_stream.str(file);
  std::pmr::string line{};
  usize pos_count{};
  usize normal_count{};
  usize uv_count{};
  usize index_count{};
  while (std::getline(file_stream, line))
  {
    if (line.starts_with(OBJ_VERTEX_HEADER)) ++pos_count;
    else if (line.starts_with(OBJ_NORMAL_HEADER)) ++normal_count;
    else if (line.starts_with(OBJ_UV_HEADER)) ++uv_count;
    else if (line.starts_with(OBJ_INDEX_HEADER)) ++index_count;
  }
  index_count *= 3;

  std::pmr::vector<Vec3> positions{};
  positions.reserve(pos_count);
  std::pmr::vector<Vec3> normals{};
  normals.reserve(normal_count);
  std::pmr::vector<Vec2> uvs{};
  uvs.reserve(uv_count);
  // TODO(szulf): is this actually better to set to index_count or am i stupid
  std::pmr::vector<Vertex> vertices{};
  vertices.reserve(index_count);
  std::pmr::vector<u32> indices{};
  indices.reserve(index_count);

  file_stream.clear();
  file_stream.str(file);
  // TODO(szulf): pull things out?
  while (std::getline(file_stream, line))
  {
    if (line.starts_with(OBJ_INDEX_HEADER))
    {
      std::size_t pos{0};
      std::size_t space_idx{line.find(' ', pos)};
      pos = space_idx + 1;

      space_idx = line.find(' ', pos);
      ERROR_ASSERT(space_idx != std::string::npos, *err, Error::ObjInvalidData, {});
      std::pmr::string part{line.substr(pos, space_idx - pos)};
      auto vertex{obj_vertex_from_index_part(part, positions, normals, uvs, &error)};
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      pos = space_idx + 1;
      obj_vertex_push_if_missing_and_push_idx(vertices, indices, vertex);

      space_idx = line.find(' ', pos);
      ERROR_ASSERT(space_idx != std::string::npos, *err, Error::ObjInvalidData, {});
      part = line.substr(pos, space_idx - pos);
      vertex = obj_vertex_from_index_part(part, positions, normals, uvs, &error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      pos = space_idx + 1;
      obj_vertex_push_if_missing_and_push_idx(vertices, indices, vertex);

      part = line.substr(pos, space_idx - pos);
      vertex = obj_vertex_from_index_part(part, positions, normals, uvs, &error);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      obj_vertex_push_if_missing_and_push_idx(vertices, indices, vertex);
    }
    else if (line.starts_with(OBJ_VERTEX_HEADER))
    {
      std::size_t pos{0};
      std::size_t space_idx{line.find(' ', pos)};
      ERROR_ASSERT(space_idx != std::string::npos, *err, Error::ObjInvalidData, {});
      pos = space_idx + 1;

      space_idx = line.find(' ', pos);
      ERROR_ASSERT(space_idx != std::string::npos, *err, Error::ObjInvalidData, {});
      f32 v1{};
      auto error{std::from_chars(line.data() + pos, line.data() + space_idx, v1).ec};
      ERROR_ASSERT(error == std::errc(), *err, Error::ObjInvalidData, {});
      pos = space_idx + 1;

      space_idx = line.find(' ', pos);
      ERROR_ASSERT(space_idx != std::string::npos, *err, Error::ObjInvalidData, {});
      f32 v2{};
      error = std::from_chars(line.data() + pos, line.data() + space_idx, v2).ec;
      ERROR_ASSERT(error == std::errc(), *err, Error::ObjInvalidData, {});
      pos = space_idx + 1;

      f32 v3{};
      error = std::from_chars(line.data() + pos, line.data() + line.size(), v3).ec;
      ERROR_ASSERT(error == std::errc(), *err, Error::ObjInvalidData, {});

      positions.push_back({v1, v2, v3});
    }
    else if (line.starts_with(OBJ_NORMAL_HEADER))
    {
      std::size_t pos{0};
      std::size_t space_idx{line.find(' ', pos)};
      ERROR_ASSERT(space_idx != std::string::npos, *err, Error::ObjInvalidData, {});
      pos = space_idx + 1;

      space_idx = line.find(' ', pos);
      ERROR_ASSERT(space_idx != std::string::npos, *err, Error::ObjInvalidData, {});
      f32 n1{};
      auto error{std::from_chars(line.data() + pos, line.data() + space_idx, n1).ec};
      ERROR_ASSERT(error == std::errc(), *err, Error::ObjInvalidData, {});
      pos = space_idx + 1;

      space_idx = line.find(' ', pos);
      ERROR_ASSERT(space_idx != std::string::npos, *err, Error::ObjInvalidData, {});
      f32 n2{};
      error = std::from_chars(line.data() + pos, line.data() + space_idx, n2).ec;
      ERROR_ASSERT(error == std::errc(), *err, Error::ObjInvalidData, {});
      pos = space_idx + 1;

      f32 n3{};
      error = std::from_chars(line.data() + pos, line.data() + line.size(), n3).ec;
      ERROR_ASSERT(error == std::errc(), *err, Error::ObjInvalidData, {});

      normals.push_back({n1, n2, n3});
    }
    else if (line.starts_with(OBJ_UV_HEADER))
    {
      std::size_t pos{0};
      std::size_t space_idx{line.find(' ', pos)};
      ERROR_ASSERT(space_idx != std::string::npos, *err, Error::ObjInvalidData, {});
      pos = space_idx + 1;

      space_idx = line.find(' ', pos);
      ERROR_ASSERT(space_idx != std::string::npos, *err, Error::ObjInvalidData, {});
      f32 u{};
      auto error{std::from_chars(line.data() + pos, line.data() + space_idx, u).ec};
      ERROR_ASSERT(error == std::errc(), *err, Error::ObjInvalidData, {});
      pos = space_idx + 1;

      f32 v{};
      error = std::from_chars(line.data() + pos, line.data() + space_idx, v).ec;
      ERROR_ASSERT(error == std::errc(), *err, Error::ObjInvalidData, {});

      uvs.push_back({u, v});
    }
    else if (line.starts_with(OBJ_MATERIAL_FILE_HEADER))
    {
      usize space_idx{line.find(' ')};
      std::pmr::string material_file_path{line.substr(space_idx + 1)};
      material_file_path.insert(0, "assets/");
      usize material_file_size;
      void* material_file = platform::read_entire_file(material_file_path.data(), &error,
                                                       &material_file_size);
      ERROR_ASSERT(error == Error::Success, *err, error, {});
      obj_parse_material_file(static_cast<const char*>(material_file), material_file_size, &error);
    }
  }

  return Mesh(vertices, indices, g_materials[0]);
}

void
Model::draw(Shader shader) const
{
  glUniformMatrix4fv(glGetUniformLocation(shader_map[static_cast<usize>(shader)], "model"),
                     1, false, model.data);

  for (const auto& mesh : meshes)
  {
    mesh.draw(shader);
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

static u32
setup_shader(const char* path, ShaderType shader_type, Error* err)
{
  Error error = Error::Success;
  const char* shader_src = static_cast<const char*>(platform::read_entire_file(path, &error));
  ERROR_ASSERT(error == Error::Success, *err, error, static_cast<u32>(-1));

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
setup_shaders(Error* err)
{
  Error error = Error::Success;

  {
    u32 v_shader = setup_shader("src/shader.vert", ShaderType::Vertex, &error);
    ASSERT(error == Error::Success, "couldnt setup vertex shader");

    u32 f_shader = setup_shader("src/shader.frag", ShaderType::Fragment, &error);
    ASSERT(error == Error::Success, "couldnt setup fragment shader");

    u32 shader = link_shaders(v_shader, f_shader, &error);
    ASSERT(error == Error::Success, "couldnt link shader");

    shader_map[static_cast<usize>(Shader::Default)] = shader;
  }

  *err = Error::Success;
}

Texture::Texture(const image::Image& img)
{
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);

  // TODO(szulf): do i want to customize these
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(img.width),
               static_cast<GLsizei>(img.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data);
  glGenerateMipmap(GL_TEXTURE_2D);
}
