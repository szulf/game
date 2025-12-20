namespace renderer
{

void init(Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  rendering.glEnable(GL_DEPTH_TEST);

  auto shader = assets::shader_from_file("shaders/shader.vert", "shaders/shader.frag", error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
  ASSERT(shader == assets::SHADER_DEFAULT, "invalid shader loading");

  shader = assets::shader_from_file("shaders/shader.vert", "shaders/green.frag", error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
  ASSERT(shader == assets::SHADER_GREEN, "invalid shader loading");

  shader = assets::shader_from_file("shaders/shader.vert", "shaders/yellow.frag", error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
  ASSERT(shader == assets::SHADER_YELLOW, "invalid shader loading");

  // TODO(szulf): change the SHADER_GREEN/YELLOW to a diffuse color in the material
  static_model_init(
    STATIC_MODEL_BOUNDING_BOX,
    assets::SHADER_GREEN,
    array_from(bounding_box_vertices, array_size(bounding_box_vertices)),
    array_from(bounding_box_indices, array_size(bounding_box_indices)),
    assets::PRIMITIVE_TRIANGLES,
    true,
    allocator
  );
  static_model_init(
    STATIC_MODEL_RING,
    assets::SHADER_YELLOW,
    array_from(ring_vertices, array_size(ring_vertices)),
    array_from(ring_indices, array_size(ring_indices)),
    assets::PRIMITIVE_LINE_STRIP,
    false,
    allocator
  );

  auto error_texture = assets::texture_make(image_error_placeholder());
  auto error_texture_handle = assets::texture_set(error_texture);
  ASSERT(error_texture_handle == 0, "error loading error texture");
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
    auto texture_id = assets::texture_get(material.texture).id;

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

    rendering.glActiveTexture(GL_TEXTURE0);
    rendering.glBindTexture(GL_TEXTURE_2D, texture_id);
    rendering.glUniform1i(rendering.glGetUniformLocation(shader, "sampler"), 0);

    // TODO(szulf): change this when actually doing light
    rendering.glUniform1i(rendering.glGetUniformLocation(shader, "emissive"), item.emissive);

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
