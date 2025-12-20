#ifndef TEXTURE_H
#define TEXTURE_H

namespace assets
{

typedef usize TextureHandle;
struct Texture
{
  u32 id;
};

Texture texture_make(const Image& img);

}

#endif
