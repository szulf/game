#include "renderer.h"

u32 shader_load_(const char* path, ShaderType shader_type, Error& out_error)
{
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());
  auto file = platform::read_file_to_string(path, scratch_arena.allocator, error);
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

  auto shader_src = file.to_cstr(scratch_arena.allocator);
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
        out_error = "Vertex shader compilation error.";
      }
      break;
      case ShaderType::FRAGMENT:
      {
        print("fragment shader compilation failed with message:\n%s\n", message);
        out_error = "Fragment shader compilation error.";
      }
      break;
      case ShaderType::GEOMETRY:
      {
        print("geometry shader compilation failed with message:\n%s\n", message);
        out_error = "Geometry shader compilation error.";
      }
      break;
    }
    return (u32) -1;
  }

  return shader;
}

Shader shader_link_(
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
    out_error = "Failed to link shaders.";
    return {};
  }
  Shader out = {};
  out.id = program;
  return out;
}

Shader shader_create_(
  const char* vert_path,
  const char* frag_path,
  const char* geom_path,
  Error& out_error
)
{
  Error error = SUCCESS;

  auto vertex_shader = shader_load_(vert_path, ShaderType::VERTEX, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  auto fragment_shader = shader_load_(frag_path, ShaderType::FRAGMENT, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});

  if (geom_path)
  {
    auto geometry_shader = shader_load_(geom_path, ShaderType::GEOMETRY, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, {});
    auto shader = shader_link_(vertex_shader, fragment_shader, true, geometry_shader, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, {});
    return {shader};
  }

  auto shader = shader_link_(vertex_shader, fragment_shader, false, 0, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  return {shader};
}

template <>
void AssetTypeGPU<ShaderHandle, Shader>::create(ShaderHandle handle)
{
  Error error = SUCCESS;
  Shader shader = {};
  switch (handle)
  {
    case ShaderHandle::DEFAULT:
    {
      shader = shader_create_("shaders/shader.vert", "shaders/default.frag", nullptr, error);
      ASSERT(error == SUCCESS, "Failed to create default shader.");
      auto index = glGetUniformBlockIndex(shader.id, "Camera");
      ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
      glUniformBlockBinding(shader.id, index, UBO_INDEX_CAMERA);
    }
    break;

    case ShaderHandle::LIGHTING:
    {
      shader = shader_create_("shaders/shader.vert", "shaders/lighting.frag", nullptr, error);
      ASSERT(error == SUCCESS, "Failed to create lighting shader.");
      auto index = glGetUniformBlockIndex(shader.id, "Camera");
      ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
      glUniformBlockBinding(shader.id, index, UBO_INDEX_CAMERA);
      index = glGetUniformBlockIndex(shader.id, "Lights");
      ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
      glUniformBlockBinding(shader.id, index, UBO_INDEX_LIGHTS);
    }
    break;

    case ShaderHandle::SHADOW_DEPTH:
    {
      shader = shader_create_(
        "shaders/shadow_depth.vert",
        "shaders/shadow_depth.frag",
        "shaders/shadow_depth.geom",
        error
      );
      ASSERT(error == SUCCESS, "Failed to create shadow depth shader.");
      auto index = glGetUniformBlockIndex(shader.id, "Camera");
      ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
      glUniformBlockBinding(shader.id, index, UBO_INDEX_CAMERA);
    }
    break;
  }
  ASSERT(error == SUCCESS, "failed to load shader: %s", error);
  data.set(handle, shader);
}

template <>
void AssetTypeGPU<ShaderHandle, Shader>::destroy(ShaderHandle handle)
{
  if (contains(handle))
  {
    glDeleteProgram(get(handle).id);
    data.remove(handle);
  }
}

template <>
void AssetTypeGPU<ShaderHandle, Shader>::destroy_all()
{
  destroy(ShaderHandle::DEFAULT);
  destroy(ShaderHandle::LIGHTING);
  destroy(ShaderHandle::SHADOW_DEPTH);
}

template <>
void AssetTypeGPU<TextureHandle, TextureGPU>::create(TextureHandle handle)
{
  auto& img = assets->textures.get(handle).image;
  TextureGPU t = {};

  glGenTextures(1, &t.id);
  glBindTexture(GL_TEXTURE_2D, t.id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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

  data.set(handle, t);
}

template <>
void AssetTypeGPU<TextureHandle, TextureGPU>::destroy(TextureHandle handle)
{
  if (contains(handle))
  {
    glDeleteTextures(1, &get(handle).id);
    data.remove(handle);
  }
}

template <>
void AssetTypeGPU<TextureHandle, TextureGPU>::destroy_all()
{
  for (usize i = 0; i < assets->textures.size; ++i)
  {
    destroy(i);
  }
}

template <>
void AssetTypeGPU<MeshHandle, MeshGPU>::create(MeshHandle handle)
{
  MeshGPU m = {};
  auto& mesh_data = assets->meshes.get(handle);
  auto& vertices = mesh_data.vertices;
  auto& indices = mesh_data.indices;

  glGenVertexArrays(1, &m.vao);
  glBindVertexArray(m.vao);

  glGenBuffers(1, &m.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    (GLsizei) (vertices.size * sizeof(Vertex)),
    vertices.data,
    GL_STATIC_DRAW
  );
  glGenBuffers(1, &m.ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    (GLsizei) (indices.size * sizeof(u32)),
    indices.data,
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
    glBindBuffer(GL_ARRAY_BUFFER, render_data->instance_data_buffer);
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

  data.set(handle, m);
}

template <>
void AssetTypeGPU<MeshHandle, MeshGPU>::destroy(MeshHandle handle)
{
  if (contains(handle) && handle >= StaticModel_COUNT)
  {
    auto& mesh = get(handle);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.ebo);
    glDeleteVertexArrays(1, &mesh.vao);
    data.remove(handle);
  }
}

template <>
void AssetTypeGPU<MeshHandle, MeshGPU>::destroy_all()
{
  for (usize i = 0; i < assets->meshes.size; ++i)
  {
    destroy(i);
  }
}

AssetsGPU AssetsGPU::make(RenderData& render_data, Assets& assets, Allocator& allocator)
{
  AssetsGPU out = {};

  out.shaders.data = Map<ShaderHandle, Shader>::make(100, allocator);
  out.shaders.render_data = &render_data;
  out.shaders.assets = &assets;

  out.textures.data = Map<TextureHandle, TextureGPU>::make(100, allocator);
  out.textures.render_data = &render_data;
  out.textures.assets = &assets;

  out.meshes.data = Map<MeshHandle, MeshGPU>::make(100, allocator);
  out.meshes.render_data = &render_data;
  out.meshes.assets = &assets;

  return out;
}

void AssetsGPU::destroy_all()
{
  shaders.destroy_all();
  textures.destroy_all();
  meshes.destroy_all();
}

mat4 get_transform_(const vec3& pos, const vec3& size, f32 rotation)
{
  auto transform = mat4::make();
  rotate(transform, rotation, {0.0f, 1.0f, 0.0f});
  translate(transform, pos);
  scale(transform, size);
  return transform;
}

void RenderPass::draw_mesh(MeshHandle handle, const vec3& pos, f32 rotation, const vec3& tint)
{
  auto transform = get_transform_(pos, {1.0f, 1.0f, 1.0f}, rotation);
  auto& mesh = assets->meshes.get(handle);
  for (usize i = 0; i < mesh.submeshes.size; ++i)
  {
    RenderItem item = {};
    item.mesh = handle;
    item.submesh_idx = i;
    item.material = mesh.submeshes[i].material;
    item.instance_data.transform = transform;
    item.instance_data.tint = tint;
    items.push(item);
  }
}

void RenderPass::draw_cube_wires(const vec3& pos, const vec3& size, const vec3& color)
{
  auto transform = get_transform_(pos, size, 0.0f);
  RenderItem item = {};
  item.mesh = StaticModel_CUBE_WIRES;
  item.submesh_idx = 0;
  item.material = assets->meshes.get(StaticModel_CUBE_WIRES).submeshes[0].material;
  item.instance_data.transform = transform;
  item.instance_data.tint = color;
  items.push(item);
}

void RenderPass::draw_ring(const vec3& pos, f32 radius, const vec3& color)
{
  auto diameter = 2.0f * radius;
  auto transform = get_transform_(pos, {diameter, 1.0f, diameter}, 0.0f);
  RenderItem item = {};
  item.mesh = StaticModel_RING;
  item.submesh_idx = 0;
  item.material = assets->meshes.get(StaticModel_RING).submeshes[0].material;
  item.instance_data.transform = transform;
  item.instance_data.tint = color;
  items.push(item);
}

void RenderPass::draw_line(const vec3& pos, f32 length, f32 rotation, const vec3& color)
{
  auto transform = get_transform_(pos, {length, 1.0f, length}, rotation);
  RenderItem item = {};
  item.mesh = StaticModel_LINE;
  item.submesh_idx = 0;
  item.material = assets->meshes.get(StaticModel_LINE).submeshes[0].material;
  item.instance_data.transform = transform;
  item.instance_data.tint = color;
  items.push(item);
}

void RenderPass::set_light(const vec3& pos, const vec3& color)
{
  light.pos = pos;
  light.color = color;
}

void RenderPass::use_shadow_map(const Camera& camera_)
{
  shadow_map_camera = &camera_;
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
  items.sort(
    +[](const RenderItem& a, const RenderItem& b) -> bool
    {
      if (a.material != b.material)
      {
        return a.material > b.material;
      }
      return a.mesh > b.mesh;
    }
  );

  switch (type)
  {
    case RenderPassType::FORWARD:
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    break;
    case RenderPassType::POINT_SHADOW_MAP:
    {
      glBindFramebuffer(GL_FRAMEBUFFER, data->shadow_framebuffer_id);
    }
    break;
  }

  glViewport(0, 0, (GLsizei) camera->viewport_width, (GLsizei) camera->viewport_height);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  {
    STD140Camera camera_std140 = {};
    camera_std140.view_pos = camera->pos;
    camera_std140.proj_view = camera->projection() * camera->look_at();
    camera_std140.far_plane = camera->far_plane;
    glBindBuffer(GL_UNIFORM_BUFFER, data->camera_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera_std140), &camera_std140);
  }

  {
    STD140Light light_std140 = {};
    light_std140.pos = {light.pos.x, light.pos.y, light.pos.z, 1.0f};
    light_std140.color = {light.color.x, light.color.y, light.color.z, 1.0f};
    light_std140.constant = Light::CONSTANT;
    light_std140.linear = Light::LINEAR;
    light_std140.quadratic = Light::QUADRATIC;
    glBindBuffer(GL_UNIFORM_BUFFER, data->lights_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(STD140Light), &light_std140);
  }

  for (usize item_idx = 0; item_idx < items.size;)
  {
    const auto& item = items[item_idx];
    const auto& mesh = assets->meshes.get(item.mesh);
    const auto& submesh = mesh.submeshes[item.submesh_idx];
    const auto& material = assets->materials.get(item.material);

    if (!assets_gpu->meshes.contains(item.mesh))
    {
      assets_gpu->meshes.create(item.mesh);
    }
    if (!assets_gpu->textures.contains(material.diffuse_map))
    {
      assets_gpu->textures.create(material.diffuse_map);
    }

    usize batch_idx = item_idx + 1;
    while ((batch_idx < items.size || batch_idx - item_idx > InstanceData::MAX) &&
           item.mesh == items[batch_idx].mesh && item.submesh_idx == items[batch_idx].submesh_idx &&
           item.material == items[batch_idx].material)
    {
      ++batch_idx;
    }
    const auto batch_size = batch_idx - item_idx;

    auto scratch_arena = ScratchArena::get();
    defer(scratch_arena.release());
    auto instance_data =
      Array<InstanceData>::make(ArrayType::STATIC, batch_size, scratch_arena.allocator);
    for (usize i = item_idx; i < batch_idx; ++i)
    {
      instance_data.push(items[i].instance_data);
    }

    ShaderHandle handle =
      type == RenderPassType::POINT_SHADOW_MAP ? ShaderHandle::SHADOW_DEPTH : material.shader;
    if (!assets_gpu->shaders.contains(handle))
    {
      assets_gpu->shaders.create(handle);
    }
    const auto shader = assets_gpu->shaders.get(handle);

    glUseProgram(shader.id);

    {
      // TODO: actually set the ambient color
      glUniform3f(glGetUniformLocation(shader.id, "material.ambient"), 0.1f, 0.1f, 0.1f);
      glUniform3f(
        glGetUniformLocation(shader.id, "material.diffuse"),
        material.diffuse_color.x,
        material.diffuse_color.y,
        material.diffuse_color.z
      );
      glUniform3f(
        glGetUniformLocation(shader.id, "material.specular"),
        material.specular_color.x,
        material.specular_color.y,
        material.specular_color.z
      );
      glUniform1f(
        glGetUniformLocation(shader.id, "material.specular_exponent"),
        material.specular_exponent
      );
      const auto& diffuse_map = assets_gpu->textures.get(material.diffuse_map);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, diffuse_map.id);
      glUniform1i(glGetUniformLocation(shader.id, "material.diffuse_map"), 0);
    }

    if (type == RenderPassType::POINT_SHADOW_MAP)
    {
      mat4 light_proj_mat = camera->projection();
      mat4 transforms[6] = {
        light_proj_mat *
          mat4::look_at(camera->pos, camera->pos + vec3{1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
        light_proj_mat *
          mat4::look_at(camera->pos, camera->pos + vec3{-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
        light_proj_mat *
          mat4::look_at(camera->pos, camera->pos + vec3{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}),
        light_proj_mat *
          mat4::look_at(camera->pos, camera->pos + vec3{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}),
        light_proj_mat *
          mat4::look_at(camera->pos, camera->pos + vec3{0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}),
        light_proj_mat *
          mat4::look_at(camera->pos, camera->pos + vec3{0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}),
      };

      glUniformMatrix4fv(
        glGetUniformLocation(shader.id, "shadow_matrices"),
        (GLsizei) 6,
        false,
        transforms[0].raw_data
      );
    }

    if (shadow_map_camera)
    {
      glActiveTexture(GL_TEXTURE1);

      glBindTexture(GL_TEXTURE_CUBE_MAP, data->shadow_cubemap.id);
      glUniform1i(glGetUniformLocation(shader.id, "shadow_map"), 1);

      glUniform1f(
        glGetUniformLocation(shader.id, "shadow_map_camera_far_plane"),
        shadow_map_camera->far_plane
      );
    }

    glBindVertexArray(assets_gpu->meshes.get(item.mesh).vao);

    glBindBuffer(GL_ARRAY_BUFFER, data->instance_data_buffer);
    glBufferSubData(
      GL_ARRAY_BUFFER,
      0,
      (GLsizeiptr) (instance_data.size * sizeof(InstanceData)),
      instance_data.data
    );

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
  const Array<Vertex>& vertices,
  const Array<u32>& indices,
  RenderPrimitive primitive,
  Assets& assets,
  Allocator& allocator
)
{
  Material material = {};
  material.shader = shader;
  material.diffuse_color = {1.0f, 1.0f, 1.0f};
  auto material_handle = assets.materials.set(material);
  MeshData mesh = {};
  mesh.vertices = vertices;
  mesh.indices = indices;
  mesh.primitive = primitive;
  mesh.submeshes = Array<Submesh>::make(ArrayType::STATIC, 1, allocator);
  mesh.submeshes.push({0, indices.size, material_handle});
  auto mesh_handle = assets.meshes.set(mesh);
  ASSERT(mesh_handle == static_model, "failed to initalize a static model");
}

Renderer Renderer::make(Assets& assets, Allocator& allocator)
{
  Renderer out = {};
  out.assets = &assets;

  glEnable(GL_DEPTH_TEST);

  glGenBuffers(1, &out.data.camera_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, out.data.camera_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Camera), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_INDEX_CAMERA, out.data.camera_ubo);

  glGenBuffers(1, &out.data.lights_ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, out.data.lights_ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Light), nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, UBO_INDEX_LIGHTS, out.data.lights_ubo);

  glGenTextures(1, &out.data.shadow_cubemap.id);
  glBindTexture(GL_TEXTURE_CUBE_MAP, out.data.shadow_cubemap.id);
  for (i32 i = 0; i < 6; ++i)
  {
    glTexImage2D(
      (GLenum) (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i),
      0,
      GL_DEPTH_COMPONENT,
      RenderData::SHADOW_CUBEMAP_WIDTH,
      RenderData::SHADOW_CUBEMAP_HEIGHT,
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

  glGenFramebuffers(1, &out.data.shadow_framebuffer_id);
  glBindFramebuffer(GL_FRAMEBUFFER, out.data.shadow_framebuffer_id);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, out.data.shadow_cubemap.id, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glGenBuffers(1, &out.data.instance_data_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, out.data.instance_data_buffer);
  glBufferData(GL_ARRAY_BUFFER, InstanceData::MAX * sizeof(InstanceData), nullptr, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  out.assets_gpu = AssetsGPU::make(out.data, *out.assets, allocator);

  static_model_init_(
    StaticModel_CUBE_WIRES,
    ShaderHandle::DEFAULT,
    Array<Vertex>::from(cube_vertices, array_size(cube_vertices)),
    Array<u32>::from(cube_wires_indices, array_size(cube_wires_indices)),
    RenderPrimitive::LINE_STRIP,
    *out.assets,
    allocator
  );
  static_model_init_(
    StaticModel_RING,
    ShaderHandle::DEFAULT,
    Array<Vertex>::from(ring_vertices, array_size(ring_vertices)),
    Array<u32>::from(ring_indices, array_size(ring_indices)),
    RenderPrimitive::LINE_STRIP,
    *out.assets,
    allocator
  );
  static_model_init_(
    StaticModel_LINE,
    ShaderHandle::DEFAULT,
    Array<Vertex>::from(line_vertices, array_size(line_vertices)),
    Array<u32>::from(line_indices, array_size(line_indices)),
    RenderPrimitive::LINE_STRIP,
    *out.assets,
    allocator
  );

  return out;
}

RenderPass Renderer::begin_pass(RenderPassType type, const Camera& camera, Allocator& allocator)
{
  RenderPass out = {};

  out.type = type;

  out.assets = assets;
  out.assets_gpu = &assets_gpu;
  out.data = &data;
  out.camera = &camera;

  out.items = Array<RenderItem>::make(ArrayType::DYNAMIC, 100, allocator);

  return out;
}
