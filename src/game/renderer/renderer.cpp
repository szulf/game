#include "renderer.h"

namespace renderer
{

ManagerGPU* ManagerGPU::instance = nullptr;
// TODO(szulf): thread_local? read from some config file?
static RenderingAPI current_rendering_api = RenderingAPI::OPENGL;

#define CAMERA_UBO_BLOCK_INDEX 0
static GLuint camera_ubo;
#define LIGHTS_UBO_BLOCK_INDEX 1
static GLuint lights_ubo;

#define SHADOW_CUBEMAP_WIDTH 1024
#define SHADOW_CUBEMAP_HEIGHT 1024
static TextureGPU shadow_cubemap;
static u32 shadow_framebuffer_id;

u32 shader_load_opengl(const char* path, ShaderType shader_type, Error& out_error)
{
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());
  usize file_size;
  void* file = platform::read_entire_file(path, scratch_arena.allocator, file_size, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});

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

  auto shader_str = String::make((const char*) file, file_size);
  auto shader_src = shader_str.to_cstr(scratch_arena.allocator);
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
      case ShaderType::VERTEX:
      {
        print("vertex shader compilation failed with message:\n%s\n", message);
        out_error = assets::SHADER_ERROR_COMPILATION;
      }
      break;
      case ShaderType::FRAGMENT:
      {
        print("fragment shader compilation failed with message:\n%s\n", message);
        out_error = assets::SHADER_ERROR_COMPILATION;
      }
      break;
      case ShaderType::GEOMETRY:
      {
        print("geometry shader compilation failed with message:\n%s\n", message);
        out_error = assets::SHADER_ERROR_COMPILATION;
      }
      break;
    }
    return (u32) -1;
  }

  return shader;
}

Shader shader_link_opengl(
  u32 vertex_shader,
  u32 fragment_shader,
  bool use_geometry,
  u32 geometry_shader,
  Error& out_error
)
{
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  if (use_geometry)
  {
    glAttachShader(program, geometry_shader);
  }
  glLinkProgram(program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  if (use_geometry)
  {
    glDeleteShader(geometry_shader);
  }

  GLint program_linked;
  glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
  if (program_linked != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetProgramInfoLog(program, 1024, &log_length, message);
    print("failed to link shaders with message:\n%s\n", message);
    out_error = assets::SHADER_ERROR_LINKING;
    return {};
  }
  Shader out = {};
  out.data.opengl = program;
  return out;
}

Shader Shader::from_file(
  const char* vert_path,
  const char* frag_path,
  const char* geom_path,
  Error& out_error
)
{
  Error error = SUCCESS;
  Shader shader;

  switch (current_rendering_api)
  {
    case RenderingAPI::OPENGL:
    {
      auto vertex_shader = shader_load_opengl(vert_path, ShaderType::VERTEX, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, {});
      auto fragment_shader = shader_load_opengl(frag_path, ShaderType::FRAGMENT, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, {});

      if (geom_path)
      {
        auto geometry_shader = shader_load_opengl(geom_path, ShaderType::GEOMETRY, error);
        ERROR_ASSERT(error == SUCCESS, out_error, error, {});
        shader = shader_link_opengl(vertex_shader, fragment_shader, true, geometry_shader, error);
        ERROR_ASSERT(error == SUCCESS, out_error, error, {});
      }
      else
      {
        shader = shader_link_opengl(vertex_shader, fragment_shader, false, 0, error);
        ERROR_ASSERT(error == SUCCESS, out_error, error, {});
      }
    }
    break;
  }

  // NOTE(szulf): stupid clang warning
  return {shader};
}

TextureGPU TextureGPU::make(assets::TextureHandle handle)
{
  TextureGPU out = {};
  auto& img = assets::texture_get(handle).img;

  switch (current_rendering_api)
  {
    case RenderingAPI::OPENGL:
    {
      auto& data = out.data.opengl;
      glGenTextures(1, &data.id);
      glBindTexture(GL_TEXTURE_2D, data.id);

      // TODO(szulf): do i want to customize these? i do for example for the error texture
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        (GLsizei) img.width,
        (GLsizei) img.height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        img.data
      );
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    break;
  }
  return out;
}

MeshGPU MeshGPU::make(assets::MeshHandle handle)
{
  MeshGPU out = {};
  auto& mesh_data = assets::mesh_get(handle);
  auto& vertices = mesh_data.vertices;
  auto& indices = mesh_data.indices;

  switch (current_rendering_api)
  {
    case RenderingAPI::OPENGL:
    {
      auto& data = out.data.opengl;
      glGenVertexArrays(1, &data.vao);
      glBindVertexArray(data.vao);

      glGenBuffers(1, &data.vbo);
      glBindBuffer(GL_ARRAY_BUFFER, data.vbo);
      glBufferData(
        GL_ARRAY_BUFFER,
        (GLsizei) (vertices.size * sizeof(Vertex)),
        vertices.data,
        GL_STATIC_DRAW
      );
      glGenBuffers(1, &data.ebo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ebo);
      glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        (GLsizei) (indices.size * sizeof(u32)),
        indices.data,
        GL_STATIC_DRAW
      );

      {
        glVertexAttribPointer(
          0,
          3,
          GL_FLOAT,
          GL_FALSE,
          sizeof(Vertex),
          (void*) offsetof(Vertex, pos)
        );
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
        glVertexAttribPointer(
          2,
          2,
          GL_FLOAT,
          GL_FALSE,
          sizeof(Vertex),
          (void*) offsetof(Vertex, uv)
        );
        glEnableVertexAttribArray(2);
      }

      {
        glBindBuffer(GL_ARRAY_BUFFER, renderer::instancing_matrix_buffer);
        // NOTE(szulf): model matrix
        glVertexAttribPointer(
          3,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(renderer::InstanceData),
          (void*) offsetof(renderer::InstanceData, model)
        );
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(
          4,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(renderer::InstanceData),
          (void*) (offsetof(renderer::InstanceData, model) + sizeof(vec4))
        );
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(
          5,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(renderer::InstanceData),
          (void*) (offsetof(renderer::InstanceData, model) + 2 * sizeof(vec4))
        );
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(
          6,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(renderer::InstanceData),
          (void*) (offsetof(renderer::InstanceData, model) + 3 * sizeof(vec4))
        );
        glEnableVertexAttribArray(6);
        // NOTE(szulf): entity tint
        glVertexAttribPointer(
          7,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(renderer::InstanceData),
          (void*) offsetof(renderer::InstanceData, tint)
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
    break;
  }

  return out;
}

ManagerGPU ManagerGPU::make(Allocator& allocator)
{
  ManagerGPU out = {};
  out.shaders = Map<assets::ShaderHandle, Shader>::make(100, allocator);
  out.textures = Map<assets::TextureHandle, TextureGPU>::make(100, allocator);
  out.meshes = Map<assets::MeshHandle, MeshGPU>::make(100, allocator);
  return out;
}

void shader_set(assets::ShaderHandle handle, const Shader& shader)
{
  ManagerGPU::instance->shaders.set(handle, shader);
}

Shader& shader_get(assets::ShaderHandle handle)
{
  return *ManagerGPU::instance->shaders[handle];
}

bool texture_gpu_exists(assets::TextureHandle handle)
{
  return ManagerGPU::instance->textures.contains(handle);
}

void texture_gpu_set(assets::TextureHandle handle, const TextureGPU& texture)
{
  ManagerGPU::instance->textures.set(handle, texture);
}

TextureGPU& texture_gpu_get(assets::TextureHandle handle)
{
  return *ManagerGPU::instance->textures[handle];
}

bool mesh_gpu_exists(assets::MeshHandle handle)
{
  return ManagerGPU::instance->meshes.contains(handle);
}

void mesh_gpu_set(assets::MeshHandle handle, const MeshGPU& mesh)
{
  ManagerGPU::instance->meshes.set(handle, mesh);
}

MeshGPU& mesh_gpu_get(assets::MeshHandle handle)
{
  return *ManagerGPU::instance->meshes[handle];
}

void static_model_init(
  StaticModel static_model,
  assets::ShaderHandle shader,
  const Array<Vertex>& vertices,
  const Array<u32>& indices,
  RenderingPrimitive primitive,
  bool wireframe,
  vec3 color,
  Allocator& allocator
)
{
  assets::Material material = {};
  material.shader = shader;
  material.wireframe = wireframe;
  material.diffuse_color = color;
  auto mesh_handle = assets::mesh_set({vertices, indices, primitive});
  // TODO(szulf): create mesh gpu here
  auto material_handle = assets::material_set(material);
  assets::Model model = {};
  model.parts = Array<assets::MeshMaterialPair>::make(ArrayType::STATIC, 1, allocator);
  model.parts.push({mesh_handle, material_handle});
  auto handle = assets::model_set(model);
  ASSERT(handle == static_model, "failed to initalize a static model");
}

Pass pass_make(Allocator& allocator)
{
  Pass out = {};
  out.items = Array<Item>::make(ArrayType::DYNAMIC, 50, allocator);
  out.lights = Array<Light>::make(ArrayType::STATIC, MAX_LIGHTS, allocator);
  return out;
}

static Vertex bounding_box_vertices[] = {
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

static u32 bounding_box_indices[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                                     12, 13, 14, 15, 16, 17, 0,  18, 1,  3,  19, 4,
                                     6,  20, 7,  9,  21, 10, 12, 22, 13, 15, 23, 16};

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
  {{0.0f, 0.0f, -0.5f}, {}, {}},
  {{0.0f, 0.0f, 0.5f},  {}, {}},
};

static u32 line_indices[] = {0, 1};

void init(Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;

  switch (current_rendering_api)
  {
    case RenderingAPI::OPENGL:
    {
      glEnable(GL_DEPTH_TEST);

      glGenBuffers(1, &camera_ubo);
      glBindBuffer(GL_UNIFORM_BUFFER, camera_ubo);
      glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Camera), nullptr, GL_DYNAMIC_DRAW);
      glBindBufferBase(GL_UNIFORM_BUFFER, CAMERA_UBO_BLOCK_INDEX, camera_ubo);

      glGenBuffers(1, &lights_ubo);
      glBindBuffer(GL_UNIFORM_BUFFER, lights_ubo);
      glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Lights), nullptr, GL_DYNAMIC_DRAW);
      glBindBufferBase(GL_UNIFORM_BUFFER, LIGHTS_UBO_BLOCK_INDEX, lights_ubo);

      glGenTextures(1, &shadow_cubemap.data.opengl.id);
      glBindTexture(GL_TEXTURE_CUBE_MAP, shadow_cubemap.data.opengl.id);
      for (i32 i = 0; i < 6; ++i)
      {
        glTexImage2D(
          (GLenum) (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
          0,
          GL_DEPTH_COMPONENT,
          SHADOW_CUBEMAP_WIDTH,
          SHADOW_CUBEMAP_HEIGHT,
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

      glGenFramebuffers(1, &shadow_framebuffer_id);
      glBindFramebuffer(GL_FRAMEBUFFER, shadow_framebuffer_id);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_cubemap.data.opengl.id, 0);
      glDrawBuffer(GL_NONE);
      glReadBuffer(GL_NONE);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      glGenBuffers(1, &instancing_matrix_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, instancing_matrix_buffer);
      glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(InstanceData), nullptr, GL_STATIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, 0);

      {
        auto shader =
          Shader::from_file("shaders/shader.vert", "shaders/default.frag", nullptr, error);
        ERROR_ASSERT(error == SUCCESS, out_error, error, );
        shader_set(assets::SHADER_DEFAULT, shader);

        auto index = glGetUniformBlockIndex(shader.data.opengl, "Camera");
        ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
        glUniformBlockBinding(shader.data.opengl, index, CAMERA_UBO_BLOCK_INDEX);
      }

      {
        auto shader =
          Shader::from_file("shaders/shader.vert", "shaders/lighting.frag", nullptr, error);
        ERROR_ASSERT(error == SUCCESS, out_error, error, );
        shader_set(assets::SHADER_LIGHTING, shader);

        auto index = glGetUniformBlockIndex(shader.data.opengl, "Camera");
        ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
        glUniformBlockBinding(shader.data.opengl, index, CAMERA_UBO_BLOCK_INDEX);
        index = glGetUniformBlockIndex(shader.data.opengl, "Lights");
        ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
        glUniformBlockBinding(shader.data.opengl, index, LIGHTS_UBO_BLOCK_INDEX);
      }

      {
        auto shader = Shader::from_file(
          "shaders/shadow_depth.vert",
          "shaders/shadow_depth.frag",
          "shaders/shadow_depth.geom",
          error
        );
        ERROR_ASSERT(error == SUCCESS, out_error, error, );
        shader_set(assets::SHADER_SHADOW_DEPTH, shader);

        auto index = glGetUniformBlockIndex(shader.data.opengl, "Camera");
        ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
        glUniformBlockBinding(shader.data.opengl, index, CAMERA_UBO_BLOCK_INDEX);
      }
    }
    break;
  }

  static_model_init(
    STATIC_MODEL_BOUNDING_BOX,
    assets::SHADER_DEFAULT,
    Array<Vertex>::from(bounding_box_vertices, array_size(bounding_box_vertices)),
    Array<u32>::from(bounding_box_indices, array_size(bounding_box_indices)),
    RenderingPrimitive::TRIANGLES,
    true,
    {0.0f, 1.0f, 0.0f},
    allocator
  );
  static_model_init(
    STATIC_MODEL_RING,
    assets::SHADER_DEFAULT,
    Array<Vertex>::from(ring_vertices, array_size(ring_vertices)),
    Array<u32>::from(ring_indices, array_size(ring_indices)),
    RenderingPrimitive::LINE_STRIP,
    false,
    {1.0f, 1.0f, 0.0f},
    allocator
  );
  static_model_init(
    STATIC_MODEL_LINE,
    assets::SHADER_DEFAULT,
    Array<Vertex>::from(line_vertices, array_size(line_vertices)),
    Array<u32>::from(line_indices, array_size(line_indices)),
    RenderingPrimitive::LINE_STRIP,
    false,
    {1.0f, 0.0f, 0.0f},
    allocator
  );
}

void queue_items(Pass& pass, const Item& render_item)
{
  if (!mesh_gpu_exists(render_item.mesh))
  {
    auto mesh_gpu = MeshGPU::make(render_item.mesh);
    mesh_gpu_set(render_item.mesh, mesh_gpu);
  }
  auto& material = assets::material_get(render_item.material);
  if (!texture_gpu_exists(material.diffuse_map))
  {
    auto texture_gpu = TextureGPU::make(material.diffuse_map);
    texture_gpu_set(material.diffuse_map, texture_gpu);
  }

  pass.items.push(render_item);
}

void queue_items(Pass& pass, const Array<Item>& render_items)
{
  for (usize i = 0; i < render_items.size; ++i)
  {
    auto& render_item = render_items[i];
    if (!mesh_gpu_exists(render_item.mesh))
    {
      auto mesh_gpu = MeshGPU::make(render_item.mesh);
      mesh_gpu_set(render_item.mesh, mesh_gpu);
    }
    auto& material = assets::material_get(render_item.material);
    if (!texture_gpu_exists(material.diffuse_map))
    {
      auto texture_gpu = TextureGPU::make(material.diffuse_map);
      texture_gpu_set(material.diffuse_map, texture_gpu);
    }

    pass.items.push(render_item);
  }
}

void sort_items(Pass& pass)
{
  pass.items.sort(
    +[](const Item& a, const Item& b) -> bool
    {
      if (a.material != b.material)
      {
        return a.material > b.material;
      }
      return a.mesh > b.mesh;
    }
  );
}

GLenum get_primitive_opengl(RenderingPrimitive primitive)
{
  switch (primitive)
  {
    case RenderingPrimitive::TRIANGLES:
      return GL_TRIANGLES;
    case RenderingPrimitive::LINE_STRIP:
      return GL_LINE_STRIP;
  }
}

void draw(const Pass& pass)
{
  switch (current_rendering_api)
  {
    case RenderingAPI::OPENGL:
    {
      glBindFramebuffer(GL_FRAMEBUFFER, pass.framebuffer_id);

      glViewport(0, 0, (GLsizei) pass.width, (GLsizei) pass.height);
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      {
        STD140Camera camera = {};
        camera.view_pos = pass.camera.pos;
        camera.proj_view = pass.camera.projection() * pass.camera.look_at();
        camera.far_plane = pass.camera.far_plane;
        glBindBuffer(GL_UNIFORM_BUFFER, camera_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera), &camera);
      }

      {
        STD140Lights lights = {};
        ASSERT(pass.lights.size < MAX_LIGHTS, "too many lights in scene");
        lights.light_count = (int) pass.lights.size;
        for (usize light_idx = 0; light_idx < pass.lights.size; ++light_idx)
        {
          auto& light = pass.lights[light_idx];
          lights.lights[light_idx].pos = {light.pos.x, light.pos.y, light.pos.z, 1.0f};
          lights.lights[light_idx].color = {light.color.x, light.color.y, light.color.z, 1.0f};
          lights.lights[light_idx].constant = 1.0f;
          lights.lights[light_idx].linear = 0.22f;
          lights.lights[light_idx].quadratic = 0.20f;
        }
        glBindBuffer(GL_UNIFORM_BUFFER, lights_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights), &lights);
      }

      for (usize item_idx = 0; item_idx < pass.items.size;)
      {
        const auto& item = pass.items[item_idx];
        usize batch_idx = item_idx + 1;
        while ((batch_idx < pass.items.size || batch_idx - item_idx > MAX_INSTANCES) &&
               (item.mesh == pass.items[batch_idx].mesh &&
                item.material == pass.items[batch_idx].material))
        {
          ++batch_idx;
        }
        auto batch_size = batch_idx - item_idx;
        auto scratch_arena = ScratchArena::get();
        defer(scratch_arena.release());
        auto instancing_data =
          Array<InstanceData>::make(ArrayType::STATIC, batch_size, scratch_arena.allocator);
        for (usize i = 0; i < batch_size; ++i)
        {
          InstanceData data = {};
          data.model = pass.items[item_idx + i].model;
          data.tint = pass.items[item_idx + i].tint;
          instancing_data.push(data);
        }

        const auto& mesh = assets::mesh_get(item.mesh);
        const auto& material = assets::material_get(item.material);
        // TODO(szulf): which one to choose?
        u32 shader = 0;
        // Shader shader;
        if (pass.override_shader)
        {
          shader = shader_get(pass.shader).data.opengl;
        }
        else
        {
          shader = shader_get(material.shader).data.opengl;
        }
        auto diffuse_map_id = texture_gpu_get(material.diffuse_map).data.opengl.id;

        if (material.wireframe)
        {
          glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        glUseProgram(shader);

        {
          glUniform3f(
            glGetUniformLocation(shader, "material.ambient"),
            material.ambient_color.x,
            material.ambient_color.y,
            material.ambient_color.z
          );
          glUniform3f(
            glGetUniformLocation(shader, "material.diffuse"),
            material.diffuse_color.x,
            material.diffuse_color.y,
            material.diffuse_color.z
          );
          glUniform3f(
            glGetUniformLocation(shader, "material.specular"),
            material.specular_color.x,
            material.specular_color.y,
            material.specular_color.z
          );
          glUniform1f(
            glGetUniformLocation(shader, "material.specular_exponent"),
            material.specular_exponent
          );
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, diffuse_map_id);
          glUniform1i(glGetUniformLocation(shader, "material.diffuse_map"), 0);
        }

        if (pass.shadow_map)
        {
          glActiveTexture(GL_TEXTURE1);

          glBindTexture(GL_TEXTURE_CUBE_MAP, pass.shadow_map->data.opengl.id);
          glUniform1i(glGetUniformLocation(shader, "shadow_map"), 1);

          glUniform1f(
            glGetUniformLocation(shader, "shadow_map_camera_far_plane"),
            pass.shadow_map_camera_far_plane
          );
        }

        if (pass.transforms)
        {
          ASSERT(pass.transforms_count == 6, "invalid transforms count");
          glUniformMatrix4fv(
            glGetUniformLocation(shader, "shadow_matrices"),
            (GLsizei) pass.transforms_count,
            false,
            pass.transforms[0].raw_data
          );
        }

        glBindVertexArray(mesh_gpu_get(item.mesh).data.opengl.vao);

        glBindBuffer(GL_ARRAY_BUFFER, instancing_matrix_buffer);
        glBufferSubData(
          GL_ARRAY_BUFFER,
          0,
          (GLsizeiptr) (instancing_data.size * sizeof(InstanceData)),
          instancing_data.data
        );

        glDrawElementsInstanced(
          get_primitive_opengl(mesh.primitive),
          (GLsizei) mesh.indices.size,
          GL_UNSIGNED_INT,
          nullptr,
          (GLsizei) batch_size
        );

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        item_idx += batch_size;
      }
    }
    break;
  }
}

}
