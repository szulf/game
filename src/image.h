#ifndef IMAGE_H
#define IMAGE_H

// NOTE(szulf): shoutout to stb_image since i pretty much just copied their implementation of this decoder

#define IMAGE_MAX_SIZE (1 << 24)

struct Image
{
  void* data;
  usize size;
  usize width;
  usize height;
  usize channels;
};

static Image image_decode_png(const char* path, Arena* temp_arena, Arena* perm_arena, Error* err);

#endif
