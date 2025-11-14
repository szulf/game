#include "renderer/texture.hpp"

#ifdef GAME_OPENGL
#  include "gl_functions.hpp"
#else
#  error Unknown rendering backend
#endif

namespace core {

#ifdef GAME_OPENGL

Texture::Texture(const Image& img) noexcept {
  glGenTextures(1, &backend_data.id);
  glBindTexture(GL_TEXTURE_2D, backend_data.id);

  // TODO(szulf): do i want to customize these
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA8,
    static_cast<GLsizei>(img.width),
    static_cast<GLsizei>(img.height),
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    img.data
  );
  glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::~Texture() {
  glDeleteTextures(1, &backend_data.id);
}

Texture::Texture(Texture&& other) : backend_data{other.backend_data} {
  other.backend_data = {};
}

Texture& Texture::operator=(Texture&& other) {
  backend_data = other.backend_data;
  other.backend_data = {};
  return *this;
}

#else
#  error Unknown rendering backend
#endif

}
