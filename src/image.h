#ifndef IMAGE_H
#define IMAGE_H

// NOTE(szulf): shoutout to stb_image since i pretty much just copied that implementation of this decoder

namespace image
{

#define IMAGE_MAX_SIZE (1 << 24)

struct Image
{
  void* data;
  usize size;
  usize width;
  usize height;
  usize channels;

  Image() {}
  void alloc_buffer(usize size);
  ~Image();
};

static Image decode_png(const char* path, Error* err);

}

#endif
