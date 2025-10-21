#ifndef IMAGE_H
#define IMAGE_H

// NOTE(szulf): shoutout to stb_image since i pretty much just copied their implementation of this decoder

namespace png
{

constexpr u32 MAX_SIZE = 1 << 24;

struct Image
{
  void* data;
  usize size;
  usize width;
  usize height;
  usize channels;
};

static Image decode(const char* path, mem::Arena& temp_arena, mem::Arena& perm_arena, Error* err);

}

#endif
