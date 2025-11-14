#include "renderer.hpp"

#include "gl_functions.hpp"
#include "asset_manager.hpp"

namespace core {

namespace renderer {

#ifdef GAME_OPENGL

void init() noexcept {
  glEnable(GL_DEPTH_TEST);
}

void clearScreen() noexcept {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void render(const Scene& scene) noexcept {
  const auto& shader_map{ShaderMap::instance()};
  const auto& assets{AssetManager::instance()};

  for (const auto& renderable : scene.renderables) {
    glUseProgram(shader_map[renderable.shader]);
    glUniformMatrix4fv(
      glGetUniformLocation(shader_map[renderable.shader], "view"),
      1,
      false,
      scene.camera.lookAtMatrix().data
    );
    glUniformMatrix4fv(
      glGetUniformLocation(shader_map[renderable.shader], "proj"),
      1,
      false,
      scene.camera.projectionMatrix().data
    );
    glUniformMatrix4fv(
      glGetUniformLocation(shader_map[renderable.shader], "model"),
      1,
      false,
      renderable.model.matrix.data
    );

    for (const auto& mesh : renderable.model.meshes) {
      glActiveTexture(GL_TEXTURE0);
      // TODO(szulf): this is horrible
      auto texture_id = assets.textures.at(assets.materials.at(mesh.material_name).texture_name).backend_data.id;
      glBindTexture(GL_TEXTURE_2D, texture_id);
      glUniform1i(glGetUniformLocation(shader_map[renderable.shader], "sampler"), 0);
      glBindVertexArray(mesh.backend_data.vao);
      glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(mesh.indices.size()),
        GL_UNSIGNED_INT,
        reinterpret_cast<void*>(0)
      );
    }
  }
}

#else
#  error Unknown rendering backend
#endif

}

}
