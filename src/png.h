#ifndef IMAGE_H
#define IMAGE_H

// NOTE(szulf): shoutout to stb_image since i pretty much just copied their implementation of this
// decoder

namespace png
{

constexpr u32 MAX_SIZE = 1 << 24;

struct Image
{
  u8* data;
  usize size;
  usize width;
  usize height;
  usize channels;

  static Image decode(const std::filesystem::path& path);

  Image(const Image& other) = delete;
  Image& operator=(const Image& other) = delete;
  Image(Image&& other);
  Image& operator=(Image&& other);
  ~Image();

private:
  Image() {}
};

}

#endif
