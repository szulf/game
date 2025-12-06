#ifndef TEXTURE_H
#define TEXTURE_H

typedef usize TextureHandle;
struct Texture
{
  u32 id;
};

Texture texture_make(const Image& img);

Texture& assets_get_texture(TextureHandle handle);
TextureHandle assets_set_texture(const Texture& texture);

#endif
