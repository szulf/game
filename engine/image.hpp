#pragma once

#include <filesystem>

namespace core {

static constexpr std::uint32_t MAX_SIZE = 1 << 24;

struct Image final {
public:
  // TODO(szulf): on fail dont throw exceptions initialize with an error image
  Image(const std::filesystem::path& path);
  ~Image();
  Image(const Image& other) = delete;
  Image& operator=(const Image& other) = delete;

  std::uint8_t* data{};
  std::size_t width{};
  std::size_t height{};
};

}
