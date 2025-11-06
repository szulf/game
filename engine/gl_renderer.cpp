#include "renderer.hpp"

#include "gl_functions.hpp"

namespace core
{

ShaderMap::ShaderMap() {}

struct Texture::BackendData
{
  GLuint id{};
};

Texture::Texture()
{
  m_backend_data = std::make_unique<BackendData>();
}

Texture::~Texture() = default;

struct Mesh::BackendData
{
  GLuint vao{};
};

Mesh::Mesh()
{
  m_backend_data = std::make_unique<BackendData>();
}

Mesh::~Mesh() = default;

auto renderer::init() -> void
{
  glEnable(GL_DEPTH_TEST);
}

auto renderer::clearScreen() -> void
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

auto renderer::render(const Scene& scene) -> void
{
  auto& shader_map{ShaderMap::getInstance()};

  for (const auto& renderable : scene.getRenderables())
  {
    glUseProgram(shader_map[renderable.shader]);
    glUniformMatrix4fv(
      glGetUniformLocation(shader_map[renderable.shader], "view"),
      1,
      false,
      scene.view().data
    );
    glUniformMatrix4fv(
      glGetUniformLocation(shader_map[renderable.shader], "proj"),
      1,
      false,
      scene.projection().data
    );
    glUniformMatrix4fv(
      glGetUniformLocation(shader_map[renderable.shader], "model"),
      1,
      false,
      renderable.model.matrix().data
    );

    for (const auto& mesh : renderable.model.meshes())
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, mesh.material().texture.backendData().id);
      glUniform1i(glGetUniformLocation(shader_map[renderable.shader], "sampler"), 0);
      glBindVertexArray(mesh.backendData().vao);
      glDrawElements(GL_TRIANGLES, (GLsizei) mesh.indices().size(), GL_UNSIGNED_INT, 0);
    }
  }
}

}
