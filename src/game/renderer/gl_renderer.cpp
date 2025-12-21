namespace renderer
{

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

static GLuint lights_ubo;

void init(Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  rendering.glEnable(GL_DEPTH_TEST);

  auto shader = assets::shader_from_file("shaders/shader.vert", "shaders/default.frag", error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
  ASSERT(shader == assets::SHADER_DEFAULT, "invalid shader loading");

  shader = assets::shader_from_file("shaders/shader.vert", "shaders/lighting.frag", error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
  ASSERT(shader == assets::SHADER_LIGHTING, "invalid shader loading");

  static_model_init(
    STATIC_MODEL_BOUNDING_BOX,
    assets::SHADER_DEFAULT,
    array_from(bounding_box_vertices, array_size(bounding_box_vertices)),
    array_from(bounding_box_indices, array_size(bounding_box_indices)),
    assets::PRIMITIVE_TRIANGLES,
    true,
    {0.0f, 1.0f, 0.0f},
    allocator
  );
  static_model_init(
    STATIC_MODEL_RING,
    assets::SHADER_DEFAULT,
    array_from(ring_vertices, array_size(ring_vertices)),
    array_from(ring_indices, array_size(ring_indices)),
    assets::PRIMITIVE_LINE_STRIP,
    false,
    {1.0f, 1.0f, 0.0f},
    allocator
  );

  auto error_texture = assets::texture_make(image_error_placeholder());
  auto error_texture_handle = assets::texture_set(error_texture);
  ASSERT(error_texture_handle == 0, "error loading error texture");

  rendering.glGenBuffers(1, &lights_ubo);
  rendering.glBindBuffer(GL_UNIFORM_BUFFER, lights_ubo);
  rendering.glBufferData(GL_UNIFORM_BUFFER, sizeof(STD140Lights), nullptr, GL_DYNAMIC_DRAW);
  rendering.glBindBufferBase(GL_UNIFORM_BUFFER, 0, lights_ubo);
}

void clear_screen()
{
  rendering.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  rendering.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void window_resize(u32 width, u32 height)
{
  rendering.glViewport(0, 0, (i32) width, (i32) height);
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
  for (usize item_idx = 0; item_idx < pass.items.size; ++item_idx)
  {
    auto& item = pass.items[item_idx];
    auto& mesh = assets::mesh_get(item.mesh);
    auto& material = assets::material_get(item.material);
    auto shader = assets::shader_get(material.shader);
    auto diffuse_map_id = assets::texture_get(material.diffuse_map).id;

    if (material.wireframe)
    {
      rendering.glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    rendering.glUseProgram(shader);
    rendering.glUniformMatrix4fv(
      rendering.glGetUniformLocation(shader, "model"),
      1,
      false,
      item.model.data
    );
    rendering
      .glUniformMatrix4fv(rendering.glGetUniformLocation(shader, "view"), 1, false, pass.view.data);
    rendering.glUniformMatrix4fv(
      rendering.glGetUniformLocation(shader, "proj"),
      1,
      false,
      pass.projection.data
    );

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

    {
      STD140Lights lights = {};
      lights.light_count = (int) pass.lights.size;
      for (usize light_idx = 0; light_idx < pass.lights.size; ++light_idx)
      {
        auto& light = pass.lights[light_idx];
        lights.lights[light_idx].pos = {light.pos.x, light.pos.y, light.pos.z, 1.0f};
        lights.lights[light_idx].color = {light.color.x, light.color.y, light.color.z, 1.0f};
        lights.lights[light_idx].constant = 1.0f;
        lights.lights[light_idx].linear = 0.14f;
        lights.lights[light_idx].quadratic = 0.07f;
      }
      rendering.glBindBuffer(GL_UNIFORM_BUFFER, lights_ubo);
      rendering.glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights), &lights);
    }

    rendering.glUniform3f(
      rendering.glGetUniformLocation(shader, "view_pos"),
      pass.view_pos.x,
      pass.view_pos.y,
      pass.view_pos.z
    );

    rendering.glBindVertexArray(mesh.vao);

    rendering.glDrawElements(
      gl_primitive_from_primitive(mesh.primitive),
      (GLsizei) mesh.index_count,
      GL_UNSIGNED_INT,
      nullptr
    );

    rendering.glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
}

}
