#ifndef IMAGE_H
#define IMAGE_H

// NOTE(szulf): shoutout to stb_image since i pretty much just copied their implementation of this decoder

#define IMAGE_MAX_SIZE (1 << 24)

typedef struct Image
{
  void* data;
  usize size;
  usize width;
  usize height;
  usize channels;
} Image;

static Image image_decode_png(void* data, usize data_size, Arena* temp_arena, Arena* perm_arena,
                              Error* err);

#endif
