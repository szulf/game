#include "renderer.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "base/enum_array.h"
#include "game/assets.h"
#include "os/gl_functions.h"

// TODO: should be owned by the Renderer class,
// but cant be bothered to work more on the renderer for now
static RenderData render_data = {};

static std::string_view shader_type_to_string(ShaderType type)
{
  switch (type)
  {
    case ShaderType::VERTEX:
      return "Vertex";
    case ShaderType::FRAGMENT:
      return "Fragment";
    case ShaderType::GEOMETRY:
      return "Geometry";
    default:
      return "Invalid";
  }
}

u32 shader_load_(const std::filesystem::path& path, ShaderType shader_type)
{
  std::ifstream file_stream{path};
  if (file_stream.fail())
  {
    throw std::runtime_error{
      std::format("[SHADER] File reading error. (path: {}).", path.string())
    };
  }
  std::stringstream ss{};
  ss << file_stream.rdbuf();
  auto file = ss.str();

  u32 shader;
  switch (shader_type)
  {
    case ShaderType::VERTEX:
    {
      shader = glCreateShader(GL_VERTEX_SHADER);
    }
    break;
    case ShaderType::FRAGMENT:
    {
      shader = glCreateShader(GL_FRAGMENT_SHADER);
    }
    break;
    case ShaderType::GEOMETRY:
    {
      shader = glCreateShader(GL_GEOMETRY_SHADER);
    }
    break;
  }

  auto shader_src = file.c_str();
  glShaderSource(shader, 1, &shader_src, nullptr);
  glCompileShader(shader);
  GLint compiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetShaderInfoLog(shader, 1024, &log_length, message);
    throw std::runtime_error{std::format(
      "Shader compilation({}) failed with a message:\n{}",
      shader_type_to_string(shader_type),
      message
    )};
  }

  return shader;
}

u32 shader_link_(u32 vertex_shader, u32 fragment_shader, std::optional<u32> geometry_shader)
{
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  if (geometry_shader)
  {
    glAttachShader(program, *geometry_shader);
  }
  glLinkProgram(program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  if (geometry_shader)
  {
    glDeleteShader(*geometry_shader);
  }

  GLint program_linked;
  glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
  if (program_linked != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetProgramInfoLog(program, 1024, &log_length, message);
    throw std::runtime_error{std::format("Failed to link shaders with message:\n{}", message)};
  }
  return program;
}

struct ShaderDescription
{
  const char* vertex{};
  const char* fragment{};
  const char* geometry{};
};

u32 shader_create_(const ShaderDescription& desc)
{
  auto vertex_shader = shader_load_(desc.vertex, ShaderType::VERTEX);
  auto fragment_shader = shader_load_(desc.fragment, ShaderType::FRAGMENT);

  if (desc.geometry)
  {
    auto geometry_shader = shader_load_(desc.geometry, ShaderType::GEOMETRY);
    auto shader = shader_link_(vertex_shader, fragment_shader, geometry_shader);
    return shader;
  }

  auto shader = shader_link_(vertex_shader, fragment_shader, std::nullopt);
  return shader;
}

constexpr static EnumArray<ShaderHandle, ShaderDescription> shader_descriptions = []()
{
  EnumArray<ShaderHandle, ShaderDescription> out{};
  out[ShaderHandle::DEFAULT] = {
    .vertex = "shaders/shader.vert",
    .fragment = "shaders/default.frag",
  };
  out[ShaderHandle::LIGHTING] = {
    .vertex = "shaders/shader.vert",
    .fragment = "shaders/lighting.frag",
  };
  out[ShaderHandle::SHADOW_DEPTH] = {
    .vertex = "shaders/shadow_depth.vert",
    .fragment = "shaders/shadow_depth.frag",
    .geometry = "shaders/shadow_depth.geom",
  };
  return out;
}();

Shader::Shader(ShaderHandle handle)
{
  auto& desc = shader_descriptions[handle];
  auto shader = shader_create_(desc);
  m_id = shader;

  auto index = glGetUniformBlockIndex(m_id, "Camera");
  if (index != GL_INVALID_INDEX)
  {
    glUniformBlockBinding(m_id, index, UBO_INDEX_CAMERA);
  }
  index = glGetUniformBlockIndex(m_id, "Lights");
  if (index != GL_INVALID_INDEX)
  {
    glUniformBlockBinding(m_id, index, UBO_INDEX_LIGHTS);
  }
}

Shader::Shader(Shader&& other)
{
  m_id = other.m_id;
  other.m_id = 0;
}

Shader& Shader::operator=(Shader&& other)
{
  if (this == &other)
  {
    return *this;
  }
  if (m_id != 0)
  {
    glDeleteProgram(m_id);
  }
  m_id = other.m_id;
  other.m_id = 0;
  return *this;
}

Shader::~Shader()
{
  glDeleteProgram(m_id);
}

static GLint gl_wrapping_option_(TextureWrappingOption option)
{
  switch (option)
  {
    case TextureWrappingOption::REPEAT:
      return GL_REPEAT;
    case TextureWrappingOption::MIRRORED_REPEAT:
      return GL_MIRRORED_REPEAT;
    case TextureWrappingOption::CLAMP_TO_EDGE:
      return GL_CLAMP_TO_EDGE;
    case TextureWrappingOption::CLAMP_TO_BORDER:
      return GL_CLAMP_TO_BORDER;
  }
}

static GLint gl_filtering_option_(TextureFilteringOption option)
{
  switch (option)
  {
    case TextureFilteringOption::LINEAR:
      return GL_LINEAR;
    case TextureFilteringOption::NEAREST:
      return GL_NEAREST;
  }
}

TextureGPU::TextureGPU(TextureHandle handle)
{
  auto& texture = AssetManager::instance().textures.get(handle);

  glGenTextures(1, &m_id);
  glBindTexture(GL_TEXTURE_2D, m_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrapping_option_(texture.wrap_s));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrapping_option_(texture.wrap_t));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filtering_option_(texture.min_filter));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filtering_option_(texture.mag_filter));

  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA8,
    (GLsizei) texture.image.width(),
    (GLsizei) texture.image.height(),
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    texture.image.data()
  );
  glGenerateMipmap(GL_TEXTURE_2D);
}

TextureGPU::TextureGPU(TextureGPU&& other)
{
  m_id = other.m_id;
  other.m_id = 0;
}

TextureGPU& TextureGPU::operator=(TextureGPU&& other)
{
  if (this == &other)
  {
    return *this;
  }
  if (m_id != 0)
  {
    glDeleteTextures(1, &m_id);
  }
  m_id = other.m_id;
  other.m_id = 0;
  return *this;
}

TextureGPU::~TextureGPU()
{
  glDeleteTextures(1, &m_id);
}

MeshGPU::MeshGPU(MeshHandle handle)
{
  auto& mesh_data = AssetManager::instance().meshes.get(handle);
  auto& vertices = mesh_data.vertices;
  auto& indices = mesh_data.indices;

  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);

  glGenBuffers(1, &m_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    (GLsizei) (vertices.size() * sizeof(Vertex)),
    vertices.data(),
    GL_STATIC_DRAW
  );
  glGenBuffers(1, &m_ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    (GLsizei) (indices.size() * sizeof(u32)),
    indices.data(),
    GL_STATIC_DRAW
  );

  {
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
      1,
      3,
      GL_FLOAT,
      GL_FALSE,
      sizeof(Vertex),
      (void*) offsetof(Vertex, normal)
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
  }

  {
    glBindBuffer(GL_ARRAY_BUFFER, render_data.instance_data_buffer);
    // NOTE: model matrix
    glVertexAttribPointer(
      3,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(InstanceData),
      (void*) offsetof(InstanceData, transform)
    );
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
      4,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(InstanceData),
      (void*) (offsetof(InstanceData, transform) + sizeof(vec4))
    );
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
      5,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(InstanceData),
      (void*) (offsetof(InstanceData, transform) + 2 * sizeof(vec4))
    );
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(
      6,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(InstanceData),
      (void*) (offsetof(InstanceData, transform) + 3 * sizeof(vec4))
    );
    glEnableVertexAttribArray(6);
    // NOTE: entity tint
    glVertexAttribPointer(
      7,
      4,
      GL_FLOAT,
      GL_FALSE,
      sizeof(InstanceData),
      (void*) offsetof(InstanceData, tint)
    );
    glEnableVertexAttribArray(7);

    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
  }

  glBindVertexArray(0);
}

MeshGPU::MeshGPU(MeshGPU&& other)
{
  m_vbo = other.m_vbo;
  other.m_vbo = 0;
  m_ebo = other.m_ebo;
  other.m_ebo = 0;
  m_vao = other.m_vao;
  other.m_vao = 0;
}

MeshGPU& MeshGPU::operator=(MeshGPU&& other)
{
  if (this == &other)
  {
    return *this;
  }
  if (m_vbo)
  {
    glDeleteBuffers(1, &m_vbo);
  }
  m_vbo = other.m_vbo;
  other.m_vbo = 0;
  if (m_ebo)
  {
    glDeleteBuffers(1, &m_ebo);
  }
  m_ebo = other.m_ebo;
  other.m_ebo = 0;
  if (m_vao)
  {
    glDeleteVertexArrays(1, &m_vao);
  }
  m_vao = other.m_vao;
  other.m_vao = 0;
  return *this;
}

MeshGPU::~MeshGPU()
{
  glDeleteBuffers(1, &m_vbo);
  glDeleteBuffers(1, &m_ebo);
  glDeleteVertexArrays(1, &m_vao);
}

mat4 get_transform_(const vec3& pos, const vec3& size, f32 rotation)
{
  mat4 transform{1.0f};
  transform = rotate(transform, rotation, {0.0f, 1.0f, 0.0f});
  transform = translate(transform, pos);
  transform = scale(transform, size);
  return transform;
}

void RenderPass::draw_mesh(MeshHandle handle, const vec3& pos, f32 rotation, const vec3& tint)
{
  auto transform = get_transform_(pos, {1.0f, 1.0f, 1.0f}, rotation);
  auto& mesh = AssetManager::instance().meshes.get(handle);
  for (usize i = 0; i < mesh.submeshes.size(); ++i)
  {
    RenderItem item = {};
    item.mesh = handle;
    item.submesh_idx = i;
    item.material = mesh.submeshes[i].material;
    item.instance_data.transform = transform;
    item.instance_data.tint = tint;
    m_items.push_back(item);
  }
}

void RenderPass::draw_cube_wires(const vec3& pos, const vec3& size, const vec3& color)
{
  auto transform = get_transform_(pos, size, 0.0f);
  RenderItem item = {};
  item.mesh = StaticModel_CUBE_WIRES;
  item.submesh_idx = 0;
  item.material = AssetManager::instance().meshes.get(StaticModel_CUBE_WIRES).submeshes[0].material;
  item.instance_data.transform = transform;
  item.instance_data.tint = color;
  m_items.push_back(item);
}

void RenderPass::draw_ring(const vec3& pos, f32 radius, const vec3& color)
{
  auto diameter = 2.0f * radius;
  auto transform = get_transform_(pos, {diameter, 1.0f, diameter}, 0.0f);
  RenderItem item = {};
  item.mesh = StaticModel_RING;
  item.submesh_idx = 0;
  item.material = AssetManager::instance().meshes.get(StaticModel_RING).submeshes[0].material;
  item.instance_data.transform = transform;
  item.instance_data.tint = color;
  m_items.push_back(item);
}

void RenderPass::draw_line(const vec3& pos, f32 length, f32 rotation, const vec3& color)
{
  auto transform = get_transform_(pos, {length, 1.0f, length}, rotation);
  RenderItem item = {};
  item.mesh = StaticModel_LINE;
  item.submesh_idx = 0;
  item.material = AssetManager::instance().meshes.get(StaticModel_LINE).submeshes[0].material;
  item.instance_data.transform = transform;
  item.instance_data.tint = color;
  m_items.push_back(item);
}

void RenderPass::set_light(const vec3& pos, const vec3& color)
{
  m_light.pos = pos;
  m_light.color = color;
}

void RenderPass::use_shadow_map(const Camera& camera_)
{
  m_shadow_map_camera = &camera_;
}

static GLenum get_primitive_(RenderPrimitive primitive)
{
  switch (primitive)
  {
    case RenderPrimitive::TRIANGLES:
    {
      return GL_TRIANGLES;
    }
    break;
    case RenderPrimitive::LINE_STRIP:
    {
      return GL_LINE_STRIP;
    }
    break;
  }
}

void RenderPass::finish()
{
  auto& assets = AssetManager::instance();
  auto& assets_gpu = AssetGPUManager::instance();
  std::ranges::sort(
    m_items,
    [](const RenderItem& a, const RenderItem& b) -> bool
    {
      if (a.material != b.material)
      {
        return a.material > b.material;
      }
      if (a.mesh != b.mesh)
      {
        return a.mesh > b.mesh;
      }
      return a.submesh_idx > b.submesh_idx;
    }
  );

  switch (m_type)
  {
    case RenderPassType::FORWARD:
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    break;
    case RenderPassType::POINT_SHADOW_MAP:
    {
      glBindFramebuffer(GL_FRAMEBUFFER, render_data.shadow_framebuffer_id);
    }
    break;
  }

  glViewport(0, 0, (GLsizei) m_camera.viewport().x, (GLsizei) m_camera.viewport().y);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  {
    STD140Camera camera_std140 = {};
    camera_std140.view_pos = m_camera.pos();
    camera_std140.proj_view = m_camera.projection() * m_camera.look_at();
    camera_std140.far_plane = m_camera.far_plane();
    glBindBuffer(GL_UNIFORM_BUFFER, render_data.camera_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera_std140), &camera_std140);
  }

  {
    STD140Light light_std140 = {};
    light_std140.pos = {m_light.pos.x, m_light.pos.y, m_light.pos.z, 1.0f};
    light_std140.color = {m_light.color.x, m_light.color.y, m_light.color.z, 1.0f};
    light_std140.constant = Light::CONSTANT;
    light_std140.linear = Light::LINEAR;
    light_std140.quadratic = Light::QUADRATIC;
    glBindBuffer(GL_UNIFORM_BUFFER, render_data.lights_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(STD140Light), &light_std140);
  }

  for (usize item_idx = 0; item_idx < m_items.size();)
  {
    const auto& item = m_items[item_idx];
    const auto& mesh = assets.meshes.get(item.mesh);
    const auto& submesh = mesh.submeshes[item.submesh_idx];
    const auto& material = assets.materials.get(item.material);

    if (!assets_gpu.meshes.contains(item.mesh))
    {
      assets_gpu.meshes.create(item.mesh);
    }
    if (!assets_gpu.textures.contains(material.diffuse_map))
    {
      assets_gpu.textures.create(material.diffuse_map);
    }

    usize batch_idx = item_idx + 1;
    while ((batch_idx < m_items.size() && batch_idx - item_idx < InstanceData::MAX) &&
           item.mesh == m_items[batch_idx].mesh &&
           item.submesh_idx == m_items[batch_idx].submesh_idx &&
           item.material == m_items[batch_idx].material)
    {
      ++batch_idx;
    }
    const auto batch_size = batch_idx - item_idx;

    std::vector<InstanceData> instance_data{};
    instance_data.reserve(batch_size);
    for (usize i = item_idx; i < batch_idx; ++i)
    {
      instance_data.push_back(m_items[i].instance_data);
    }

    ShaderHandle handle =
      m_type == RenderPassType::POINT_SHADOW_MAP ? ShaderHandle::SHADOW_DEPTH : material.shader;
    if (!assets_gpu.shaders.contains(handle))
    {
      assets_gpu.shaders.create(handle);
    }
    const auto& shader = assets_gpu.shaders.get(handle);

    glUseProgram(shader.handle());

    {
      glUniform3f(
        glGetUniformLocation(shader.handle(), "material.ambient"),
        m_ambient_color.x,
        m_ambient_color.y,
        m_ambient_color.z
      );
      glUniform3f(
        glGetUniformLocation(shader.handle(), "material.diffuse"),
        material.diffuse_color.x,
        material.diffuse_color.y,
        material.diffuse_color.z
      );
      glUniform3f(
        glGetUniformLocation(shader.handle(), "material.specular"),
        material.specular_color.x,
        material.specular_color.y,
        material.specular_color.z
      );
      glUniform1f(
        glGetUniformLocation(shader.handle(), "material.specular_exponent"),
        material.specular_exponent
      );
      const auto& diffuse_map = assets_gpu.textures.get(material.diffuse_map);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, diffuse_map.handle());
      glUniform1i(glGetUniformLocation(shader.handle(), "material.diffuse_map"), 0);
    }

    if (m_type == RenderPassType::POINT_SHADOW_MAP)
    {
      mat4 light_proj_mat = m_camera.projection();
      mat4 transforms[6] = {
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{1.0f, 0.0f, 0.0f},
                           {0.0f, -1.0f, 0.0f}
                         ),
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{-1.0f, 0.0f, 0.0f},
                           {0.0f, -1.0f, 0.0f}
                         ),
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{0.0f, 1.0f, 0.0f},
                           {0.0f, 0.0f, 1.0f}
                         ),
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{0.0f, -1.0f, 0.0f},
                           {0.0f, 0.0f, -1.0f}
                         ),
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{0.0f, 0.0f, 1.0f},
                           {0.0f, -1.0f, 0.0f}
                         ),
        light_proj_mat * mat4::look_at(
                           m_camera.pos(),
                           m_camera.pos() + vec3{0.0f, 0.0f, -1.0f},
                           {0.0f, -1.0f, 0.0f}
                         ),
      };

      glUniformMatrix4fv(
        glGetUniformLocation(shader.handle(), "shadow_matrices"),
        (GLsizei) 6,
        false,
        transforms[0].data()
      );
    }

    if (m_shadow_map_camera)
    {
      glActiveTexture(GL_TEXTURE1);

      glBindTexture(GL_TEXTURE_CUBE_MAP, render_data.shadow_cubemap);
      glUniform1i(glGetUniformLocation(shader.handle(), "shadow_map"), 1);

      glUniform1f(
        glGetUniformLocation(shader.handle(), "shadow_map_camera_far_plane"),
        m_shadow_map_camera->far_plane()
      );
    }

    glBindBuffer(GL_ARRAY_BUFFER, render_data.instance_data_buffer);
    glBufferSubData(
      GL_ARRAY_BUFFER,
      0,
      (GLsizeiptr) (instance_data.size() * sizeof(InstanceData)),
      instance_data.data()
    );

    glBindVertexArray(assets_gpu.meshes.get(item.mesh).handle());

    glDrawElementsInstanced(
      get_primitive_(mesh.primitive),
      (GLsizei) submesh.index_count,
      GL_UNSIGNED_INT,
      (void*) (submesh.index_offset * sizeof(u32)),
      (GLsizei) batch_size
    );

    item_idx += batch_size;
  }
}

static Vertex cube_vertices[] = {
  {{-0.500000, 0.500000, 0.500000},   {-1.000000, -0.000000, -0.000000}, {0.000000, 1.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-1.000000, -0.000000, -0.000000}, {1.000000, 0.000000}},
  {{-0.500000, -0.500000, 0.500000},  {-1.000000, -0.000000, -0.000000}, {0.000000, 0.000000}},
  {{-0.500000, 0.500000, -0.500000},  {-0.000000, -0.000000, -1.000000}, {0.000000, 1.000000}},
  {{0.500000, -0.500000, -0.500000},  {-0.000000, -0.000000, -1.000000}, {1.000000, 0.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-0.000000, -0.000000, -1.000000}, {0.000000, 0.000000}},
  {{0.500000, 0.500000, -0.500000},   {1.000000, -0.000000, -0.000000},  {1.000000, 1.000000}},
  {{0.500000, -0.500000, 0.500000},   {1.000000, -0.000000, -0.000000},  {0.000000, 0.000000}},
  {{0.500000, -0.500000, -0.500000},  {1.000000, -0.000000, -0.000000},  {1.000000, 0.000000}},
  {{0.500000, 0.500000, 0.500000},    {-0.000000, -0.000000, 1.000000},  {1.000000, 1.000000}},
  {{-0.500000, -0.500000, 0.500000},  {-0.000000, -0.000000, 1.000000},  {0.000000, 0.000000}},
  {{0.500000, -0.500000, 0.500000},   {-0.000000, -0.000000, 1.000000},  {1.000000, 0.000000}},
  {{0.500000, -0.500000, -0.500000},  {-0.000000, -1.000000, -0.000000}, {1.000000, 1.000000}},
  {{-0.500000, -0.500000, 0.500000},  {-0.000000, -1.000000, -0.000000}, {0.000000, 0.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-0.000000, -1.000000, -0.000000}, {0.000000, 1.000000}},
  {{-0.500000, 0.500000, -0.500000},  {-0.000000, 1.000000, -0.000000},  {0.000000, 1.000000}},
  {{0.500000, 0.500000, 0.500000},    {-0.000000, 1.000000, -0.000000},  {1.000000, 0.000000}},
  {{0.500000, 0.500000, -0.500000},   {-0.000000, 1.000000, -0.000000},  {1.000000, 1.000000}},
  {{-0.500000, 0.500000, -0.500000},  {-1.000000, -0.000000, -0.000000}, {1.000000, 1.000000}},
  {{0.500000, 0.500000, -0.500000},   {-0.000000, -0.000000, -1.000000}, {1.000000, 1.000000}},
  {{0.500000, 0.500000, 0.500000},    {1.000000, -0.000000, -0.000000},  {0.000000, 1.000000}},
  {{-0.500000, 0.500000, 0.500000},   {-0.000000, -0.000000, 1.000000},  {0.000000, 1.000000}},
  {{0.500000, -0.500000, 0.500000},   {-0.000000, -1.000000, -0.000000}, {1.000000, 0.000000}},
  {{-0.500000, 0.500000, 0.500000},   {-0.000000, 1.000000, -0.000000},  {0.000000, 0.000000}},
};

static u32 cube_wires_indices[] = {1, 2, 7, 4, 1, 3, 21, 2, 21, 9, 7, 9, 17, 4, 17, 3};

static Vertex ring_vertices[] = {
  {{0.000000f, 0.000000f, -0.500000f},  {}, {}},
  {{-0.097545f, 0.000000f, -0.490393f}, {}, {}},
  {{-0.191342f, 0.000000f, -0.461940f}, {}, {}},
  {{-0.277785f, 0.000000f, -0.415735f}, {}, {}},
  {{-0.353553f, 0.000000f, -0.353553f}, {}, {}},
  {{-0.415735f, 0.000000f, -0.277785f}, {}, {}},
  {{-0.461940f, 0.000000f, -0.191342f}, {}, {}},
  {{-0.490393f, 0.000000f, -0.097545f}, {}, {}},
  {{-0.500000f, 0.000000f, 0.000000f},  {}, {}},
  {{-0.490393f, 0.000000f, 0.097545f},  {}, {}},
  {{-0.461940f, 0.000000f, 0.191342f},  {}, {}},
  {{-0.415735f, 0.000000f, 0.277785f},  {}, {}},
  {{-0.353553f, 0.000000f, 0.353553f},  {}, {}},
  {{-0.277785f, 0.000000f, 0.415735f},  {}, {}},
  {{-0.191342f, 0.000000f, 0.461940f},  {}, {}},
  {{-0.097545f, 0.000000f, 0.490393f},  {}, {}},
  {{0.000000f, 0.000000f, 0.500000f},   {}, {}},
  {{0.097545f, 0.000000f, 0.490393f},   {}, {}},
  {{0.191342f, 0.000000f, 0.461940f},   {}, {}},
  {{0.277785f, 0.000000f, 0.415735f},   {}, {}},
  {{0.353553f, 0.000000f, 0.353553f},   {}, {}},
  {{0.415735f, 0.000000f, 0.277785f},   {}, {}},
  {{0.461940f, 0.000000f, 0.191342f},   {}, {}},
  {{0.490393f, 0.000000f, 0.097545f},   {}, {}},
  {{0.500000f, 0.000000f, 0.000000f},   {}, {}},
  {{0.490393f, 0.000000f, -0.097545f},  {}, {}},
  {{0.461940f, 0.000000f, -0.191342f},  {}, {}},
  {{0.415735f, 0.000000f, -0.277785f},  {}, {}},
  {{0.353553f, 0.000000f, -0.353553f},  {}, {}},
  {{0.277785f, 0.000000f, -0.415735f},  {}, {}},
  {{0.191342f, 0.000000f, -0.461940f},  {}, {}},
  {{0.097545f, 0.000000f, -0.490393f},  {}, {}},
};

static u32 ring_indices[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                             17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0};

static Vertex line_vertices[] = {
  {{0.0f, 0.0f, 0.0f}, {}, {}},
  {{0.0f, 0.0f, 1.0f}, {}, {}},
};

static u32 line_indices[] = {0, 1};

void static_model_init_(
  StaticModel static_model,
  ShaderHandle shader,
  std::vector<Vertex>&& vertices,
  std::vector<u32>&& indices,
  RenderPrimitive primitive
)
{
  Material material = {};
  material.shader = shader;
  material.diffuse_color = {1.0f, 1.0f, 1.0f};
  auto material_handle = AssetManager::instance().materials.set(std::move(material));
  MeshData mesh = {};
  mesh.vertices = std::move(vertices);
  mesh.indices = std::move(indices);
  mesh.primitive = primitive;
  mesh.submeshes.push_back({0, mesh.indices.size(), material_handle});
  auto mesh_handle = AssetManager::instance().meshes.set(std::move(mesh));
  ASSERT(mesh_handle == static_model, "failed to initalize a static model");
}

Renderer::Renderer()
{
  glEnable(GL_DEPTH_TEST);

  glGenBuffers(1, &render_data.camera_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, render_data.camera_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Camera), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_INDEX_CAMERA, render_data.camera_ubo);

  glGenBuffers(1, &render_data.lights_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, render_data.lights_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Light), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_INDEX_LIGHTS, render_data.lights_ubo);

  glGenTextures(1, &render_data.shadow_cubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, render_data.shadow_cubemap);
  for (i32 i = 0; i < 6; ++i)
  {
    glTexImage2D(
      (GLenum) (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
      0,
      GL_DEPTH_COMPONENT,
      RenderData::SHADOW_CUBEMAP_DIMENSIONS.x,
      RenderData::SHADOW_CUBEMAP_DIMENSIONS.y,
      0,
      GL_DEPTH_COMPONENT,
      GL_FLOAT,
      nullptr
    );
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glGenFramebuffers(1, &render_data.shadow_framebuffer_id);
  glBindFramebuffer(GL_FRAMEBUFFER, render_data.shadow_framebuffer_id);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, render_data.shadow_cubemap, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glGenBuffers(1, &render_data.instance_data_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, render_data.instance_data_buffer);
  glBufferData(GL_ARRAY_BUFFER, InstanceData::MAX * sizeof(InstanceData), nullptr, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  static_model_init_(
    StaticModel_CUBE_WIRES,
    ShaderHandle::DEFAULT,
    std::vector<Vertex>{cube_vertices, cube_vertices + ARRAY_SIZE(cube_vertices)},
    std::vector<u32>{cube_wires_indices, cube_wires_indices + ARRAY_SIZE(cube_wires_indices)},
    RenderPrimitive::LINE_STRIP
  );
  static_model_init_(
    StaticModel_RING,
    ShaderHandle::DEFAULT,
    std::vector<Vertex>{ring_vertices, ring_vertices + ARRAY_SIZE(ring_vertices)},
    std::vector<u32>{ring_indices, ring_indices + ARRAY_SIZE(ring_indices)},
    RenderPrimitive::LINE_STRIP
  );
  static_model_init_(
    StaticModel_LINE,
    ShaderHandle::DEFAULT,
    std::vector<Vertex>{line_vertices, line_vertices + ARRAY_SIZE(line_vertices)},
    std::vector<u32>{line_indices, line_indices + ARRAY_SIZE(line_indices)},
    RenderPrimitive::LINE_STRIP
  );
}

RenderPass
Renderer::begin_pass(RenderPassType type, const Camera& camera, const vec3& ambient_color)
{
  return {type, camera, ambient_color};
}
