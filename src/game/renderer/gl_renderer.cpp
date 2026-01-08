namespace renderer
{

struct STD140Camera
{
  mat4 proj_view;
  vec3 view_pos;
  float far_plane;
};

struct STD140Light
{
  vec4 pos;
  vec4 color;

  f32 constant;
  f32 linear;
  f32 quadratic;
  f32 _pad;
};

struct STD140Lights
{
  int light_count;
  vec3 _pad;
  STD140Light lights[MAX_LIGHTS];
};

#define CAMERA_UBO_BLOCK_INDEX 0
static GLuint camera_ubo;
#define LIGHTS_UBO_BLOCK_INDEX 1
static GLuint lights_ubo;

static assets::Texture shadow_cubemap;
static u32 shadow_framebuffer_id;

#define MAX_INSTANCES 10000
// TODO(szulf): this should be here, for now is in assets/mesh.cpp
// static u32 instancing_matrix_buffer;

void init(Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  rendering.glEnable(GL_DEPTH_TEST);

  rendering.glGenBuffers(1, &camera_ubo);
  rendering.glBindBuffer(GL_UNIFORM_BUFFER, camera_ubo);
  rendering.glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Camera), nullptr, GL_DYNAMIC_DRAW);
  rendering.glBindBufferBase(GL_UNIFORM_BUFFER, CAMERA_UBO_BLOCK_INDEX, camera_ubo);

  rendering.glGenBuffers(1, &lights_ubo);
  rendering.glBindBuffer(GL_UNIFORM_BUFFER, lights_ubo);
  rendering.glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Lights), nullptr, GL_DYNAMIC_DRAW);
  rendering.glBindBufferBase(GL_UNIFORM_BUFFER, LIGHTS_UBO_BLOCK_INDEX, lights_ubo);

#define SHADOW_CUBEMAP_WIDTH 1024
#define SHADOW_CUBEMAP_HEIGHT 1024
  rendering.glGenTextures(1, &shadow_cubemap.id);
  rendering.glBindTexture(GL_TEXTURE_CUBE_MAP, shadow_cubemap.id);
  for (i32 i = 0; i < 6; ++i)
  {
    rendering.glTexImage2D(
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
  rendering.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  rendering.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  rendering.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  rendering.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  rendering.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  rendering.glGenFramebuffers(1, &shadow_framebuffer_id);
  rendering.glBindFramebuffer(GL_FRAMEBUFFER, shadow_framebuffer_id);
  rendering.glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_cubemap.id, 0);
  rendering.glDrawBuffer(GL_NONE);
  rendering.glReadBuffer(GL_NONE);
  rendering.glBindFramebuffer(GL_FRAMEBUFFER, 0);

  rendering.glGenBuffers(1, &instancing_matrix_buffer);
  rendering.glBindBuffer(GL_ARRAY_BUFFER, instancing_matrix_buffer);
  rendering
    .glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(InstanceData), nullptr, GL_STATIC_DRAW);
  rendering.glBindBuffer(GL_ARRAY_BUFFER, 0);

  {
    auto shader_handle =
      assets::shader_from_file("shaders/shader.vert", "shaders/default.frag", nullptr, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, );
    ASSERT(shader_handle == assets::SHADER_DEFAULT, "invalid shader loading");
    auto shader = assets::shader_get(shader_handle);

    auto index = rendering.glGetUniformBlockIndex(shader, "Camera");
    ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
    rendering.glUniformBlockBinding(shader, index, CAMERA_UBO_BLOCK_INDEX);
  }

  {
    auto shader_handle =
      assets::shader_from_file("shaders/shader.vert", "shaders/lighting.frag", nullptr, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, );
    ASSERT(shader_handle == assets::SHADER_LIGHTING, "invalid shader loading");
    auto shader = assets::shader_get(shader_handle);

    auto index = rendering.glGetUniformBlockIndex(shader, "Camera");
    ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
    rendering.glUniformBlockBinding(shader, index, CAMERA_UBO_BLOCK_INDEX);
    index = rendering.glGetUniformBlockIndex(shader, "Lights");
    ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
    rendering.glUniformBlockBinding(shader, index, LIGHTS_UBO_BLOCK_INDEX);
  }

  {
    auto shader_handle = assets::shader_from_file(
      "shaders/shadow_depth.vert",
      "shaders/shadow_depth.frag",
      "shaders/shadow_depth.geom",
      error
    );
    ERROR_ASSERT(error == SUCCESS, out_error, error, );
    ASSERT(shader_handle == assets::SHADER_SHADOW_DEPTH, "invalid shader loading");
    auto shader = assets::shader_get(shader_handle);

    auto index = rendering.glGetUniformBlockIndex(shader, "Camera");
    ASSERT(index != GL_INVALID_INDEX, "invalid unifrom block index");
    rendering.glUniformBlockBinding(shader, index, CAMERA_UBO_BLOCK_INDEX);
  }

  static_model_init(
    STATIC_MODEL_BOUNDING_BOX,
    assets::SHADER_DEFAULT,
    Array<Vertex>::from(bounding_box_vertices, array_size(bounding_box_vertices)),
    Array<u32>::from(bounding_box_indices, array_size(bounding_box_indices)),
    assets::PRIMITIVE_TRIANGLES,
    true,
    {0.0f, 1.0f, 0.0f},
    allocator
  );
  static_model_init(
    STATIC_MODEL_RING,
    assets::SHADER_DEFAULT,
    Array<Vertex>::from(ring_vertices, array_size(ring_vertices)),
    Array<u32>::from(ring_indices, array_size(ring_indices)),
    assets::PRIMITIVE_LINE_STRIP,
    false,
    {1.0f, 1.0f, 0.0f},
    allocator
  );
}

GLenum gl_primitive_from_primitive(assets::Primitive primitive)
{
  switch (primitive)
  {
    case assets::PRIMITIVE_TRIANGLES:
      return GL_TRIANGLES;
    case assets::PRIMITIVE_LINE_STRIP:
      return GL_LINE_STRIP;
  }
}

void draw(const Pass& pass)
{
  rendering.glBindFramebuffer(GL_FRAMEBUFFER, pass.framebuffer_id);

  rendering.glViewport(0, 0, (GLsizei) pass.width, (GLsizei) pass.height);
  rendering.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  rendering.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  {
    STD140Camera camera = {};
    camera.view_pos = pass.camera.pos;
    camera.proj_view = pass.camera.projection() * pass.camera.look_at();
    camera.far_plane = pass.camera.far_plane;
    rendering.glBindBuffer(GL_UNIFORM_BUFFER, camera_ubo);
    rendering.glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera), &camera);
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
    rendering.glBindBuffer(GL_UNIFORM_BUFFER, lights_ubo);
    rendering.glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights), &lights);
  }

  for (usize item_idx = 0; item_idx < pass.items.size;)
  {
    auto& item = pass.items[item_idx];
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

    auto& mesh = assets::mesh_get(item.mesh);
    auto& material = assets::material_get(item.material);
    assets::Shader shader;
    if (pass.override_shader)
    {
      shader = assets::shader_get(pass.shader);
    }
    else
    {
      shader = assets::shader_get(material.shader);
    }
    auto diffuse_map_id = assets::texture_get(material.diffuse_map).id;

    if (material.wireframe)
    {
      rendering.glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    rendering.glUseProgram(shader);

    {
      rendering.glUniform3f(
        rendering.glGetUniformLocation(shader, "material.ambient"),
        material.ambient_color.x,
        material.ambient_color.y,
        material.ambient_color.z
      );
      rendering.glUniform3f(
        rendering.glGetUniformLocation(shader, "material.diffuse"),
        material.diffuse_color.x,
        material.diffuse_color.y,
        material.diffuse_color.z
      );
      rendering.glUniform3f(
        rendering.glGetUniformLocation(shader, "material.specular"),
        material.specular_color.x,
        material.specular_color.y,
        material.specular_color.z
      );
      rendering.glUniform1f(
        rendering.glGetUniformLocation(shader, "material.specular_exponent"),
        material.specular_exponent
      );
      rendering.glActiveTexture(GL_TEXTURE0);
      rendering.glBindTexture(GL_TEXTURE_2D, diffuse_map_id);
      rendering.glUniform1i(rendering.glGetUniformLocation(shader, "material.diffuse_map"), 0);
    }

    if (pass.shadow_map)
    {
      rendering.glActiveTexture(GL_TEXTURE1);
      rendering.glBindTexture(GL_TEXTURE_CUBE_MAP, pass.shadow_map->id);
      rendering.glUniform1i(rendering.glGetUniformLocation(shader, "shadow_map"), 1);

      rendering.glUniform1f(
        rendering.glGetUniformLocation(shader, "shadow_map_camera_far_plane"),
        pass.shadow_map_camera_far_plane
      );
    }

    if (pass.transforms)
    {
      ASSERT(pass.transforms_count == 6, "invalid transforms count");
      rendering.glUniformMatrix4fv(
        rendering.glGetUniformLocation(shader, "shadow_matrices"),
        (GLsizei) pass.transforms_count,
        false,
        pass.transforms[0].raw_data
      );
    }

    rendering.glBindVertexArray(mesh.vao);

    rendering.glBindBuffer(GL_ARRAY_BUFFER, instancing_matrix_buffer);
    rendering.glBufferSubData(
      GL_ARRAY_BUFFER,
      0,
      (GLsizeiptr) (instancing_data.size * sizeof(InstanceData)),
      instancing_data.data
    );

    rendering.glDrawElementsInstanced(
      gl_primitive_from_primitive(mesh.primitive),
      (GLsizei) mesh.index_count,
      GL_UNSIGNED_INT,
      nullptr,
      (GLsizei) batch_size
    );

    rendering.glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    item_idx += batch_size;
  }
}

}
