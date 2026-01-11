#ifndef TEXTURE_H
#define TEXTURE_H

namespace assets
{

enum class TextureWrappingOptions
{
  REPEAT,
  MIRRORED_REPEAT,
  CLAMP_TO_EDGE,
  CLAMP_TO_BORDER,
};

enum class TextureFilteringOptions
{
  LINEAR,
  NEAREST,
};

struct TextureOptions
{
  TextureWrappingOptions wrapping_x;
  TextureWrappingOptions wrapping_y;
  TextureFilteringOptions filter_magnifying;
  TextureFilteringOptions filter_minifying;
};

typedef usize TextureHandle;
struct Texture
{
  Image img;
  TextureOptions options;
};

}

#endif
