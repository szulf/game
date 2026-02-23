#ifndef IMAGE_H
#define IMAGE_H

#include <filesystem>

#include "base/base.h"
#include "base/math.h"

// TODO: error placeholder
struct Image
{
  Image(const std::filesystem::path& path);
  Image(const Image& other) = delete;
  Image& operator=(const Image& other) = delete;
  Image(Image&& other);
  Image& operator=(Image&& other);
  ~Image();

  [[nodiscard]] constexpr inline u32 width() const
  {
    return m_dimensions.x;
  }

  [[nodiscard]] constexpr inline u32 height() const
  {
    return m_dimensions.y;
  }

  [[nodiscard]] constexpr inline void* data() const
  {
    return m_data;
  }

private:
  u8* m_data{};
  uvec2 m_dimensions{};
};

#endif
