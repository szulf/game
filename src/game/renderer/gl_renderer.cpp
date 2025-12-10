void renderer_init(Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  rendering.glEnable(GL_DEPTH_TEST);

  auto shader = shader_from_file("shaders/shader.vert", "shaders/green.frag", error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
  ASSERT(shader == SHADER_GREEN, "invalid shader loading");

  shader = shader_from_file("shaders/shader.vert", "shaders/yellow.frag", error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
  ASSERT(shader == SHADER_YELLOW, "invalid shader loading");

  shader = shader_from_file("shaders/shader.vert", "shaders/shader.frag", error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
  ASSERT(shader == SHADER_DEFAULT, "invalid shader loading");

  static_model_init(
    STATIC_MODEL_BOUNDING_BOX,
    SHADER_GREEN,
    array_from(bounding_box_vertices, array_size(bounding_box_vertices)),
    array_from(bounding_box_indices, array_size(bounding_box_indices)),
    allocator
  );
  static_model_init(
    STATIC_MODEL_RING,
    SHADER_YELLOW,
    array_from(ring_vertices, array_size(ring_vertices)),
    array_from(ring_indices, array_size(ring_indices)),
    allocator
  );
}

void renderer_clear_screen()
{
  rendering.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  rendering.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer_window_resize(u32 width, u32 height)
{
  rendering.glViewport(0, 0, (i32) width, (i32) height);
}

void renderer_draw(Array<DrawCall>& queue)
{
  for (usize draw_call_idx = 0; draw_call_idx < queue.size; ++draw_call_idx)
  {
    auto& draw_call = queue[draw_call_idx];
    if (draw_call.wireframe)
    {
      rendering.glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    auto& model = assets_get_model(draw_call.model_handle);

    for (usize mesh_idx = 0; mesh_idx < model.meshes.size; ++mesh_idx)
    {
      auto& mesh = assets_get_mesh(model.meshes[mesh_idx]);
      auto& material = assets_get_material(mesh.material);
      auto shader = assets_get_shader(material.shader);

      rendering.glUseProgram(assets_get_shader(material.shader));
      rendering.glUniformMatrix4fv(
        rendering.glGetUniformLocation(shader, "model"),
        1,
        false,
        draw_call.model.data
      );
      rendering.glUniformMatrix4fv(
        rendering.glGetUniformLocation(shader, "view"),
        1,
        false,
        draw_call.view.data
      );
      rendering.glUniformMatrix4fv(
        rendering.glGetUniformLocation(shader, "proj"),
        1,
        false,
        draw_call.projection.data
      );

      if (material.texture)
      {
        auto texture_id = assets_get_texture(material.texture).id;
        rendering.glActiveTexture(GL_TEXTURE0);
        rendering.glBindTexture(GL_TEXTURE_2D, texture_id);
        rendering.glUniform1i(rendering.glGetUniformLocation(shader, "sampler"), 0);
      }

      rendering.glUniform1i(rendering.glGetUniformLocation(shader, "emissive"), draw_call.emissive);

      rendering.glBindVertexArray(mesh.vao);

      switch (draw_call.primitive)
      {
        case PRIMITIVE_TRIANGLES:
        {
          rendering
            .glDrawElements(GL_TRIANGLES, (GLsizei) mesh.indices.size, GL_UNSIGNED_INT, nullptr);
        }
        break;
        case PRIMITIVE_LINE_STRIP:
        {
          rendering
            .glDrawElements(GL_LINE_STRIP, (GLsizei) mesh.indices.size, GL_UNSIGNED_INT, nullptr);
        }
        break;
      }
    }
    if (draw_call.wireframe)
    {
      rendering.glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  }
  queue.size = 0;
}
