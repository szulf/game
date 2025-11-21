#include "engine/renderer/texture.hpp"

#ifdef GAME_OPENGL
#  include "gl_functions.hpp"
#else
#  error Unknown rendering backend
#endif

namespace core {

#ifdef GAME_OPENGL

Texture Texture::make(const Image& img) {
  Texture t;
  glGenTextures(1, &t.backend_data.id);
  glBindTexture(GL_TEXTURE_2D, t.backend_data.id);

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
  return t;
}

#else
#  error Unknown rendering backend
#endif

}
