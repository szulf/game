#ifndef IMAGE_H
#define IMAGE_H

// NOTE(szulf): shoutout to stb_image since i pretty much just copied that implementation of this decoder

namespace image {

static constexpr std::uint32_t MAX_SIZE = 1 << 24;

struct Image {
  Image() {}

  static constexpr std::uint8_t channels = 4;
  std::vector<std::uint8_t> data{};
  std::size_t width{};
  std::size_t height{};
  std::size_t size{};
};

static Image decode_png(const std::filesystem::path& path, Error* err);

}

#endif
