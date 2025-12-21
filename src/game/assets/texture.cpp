#include "texture.h"

namespace assets
{

Texture texture_make(const Image& img)
{
  Texture out = {};
  rendering.glGenTextures(1, &out.id);
  rendering.glBindTexture(GL_TEXTURE_2D, out.id);

  // TODO(szulf): do i want to customize these? i do for example for the error texture
  rendering.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  rendering.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  rendering.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  rendering.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  rendering.glTexImage2D(
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
  rendering.glGenerateMipmap(GL_TEXTURE_2D);
  return out;
}

}
