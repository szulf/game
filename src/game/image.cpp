#include "image.h"

#include <cstring>

#include "stb/image.h"

Image::Image(Image&& other)
{
  m_data = other.m_data;
  other.m_data = nullptr;
  m_dimensions = other.m_dimensions;
}

Image& Image::operator=(Image&& other)
{
  if (this == &other)
  {
    return *this;
  }
  if (m_data)
  {
    stbi_image_free(m_data);
  }
  m_data = other.m_data;
  other.m_data = nullptr;
  m_dimensions = other.m_dimensions;
  return *this;
}

Image::~Image()
{
  if (m_data)
  {
    stbi_image_free(m_data);
  }
}

Image Image::from_file(const std::filesystem::path& path)
{
  Image out{};
  int width, height, channels;
  out.m_data = stbi_load(path.c_str(), &width, &height, &channels, 4);
  ASSERT(channels == 4, "Invalid image file.");
  out.m_dimensions = {static_cast<u32>(width), static_cast<u32>(height)};
  return out;
}

static u8 error_placeholder_data[] = {
  // clang-format off
  0x00, 0x00, 0x00, 0xFF,
  0xFC, 0x0F, 0xC0, 0xFF,
  0xFC, 0x0F, 0xC0, 0xFF,
  0x00, 0x00, 0x00, 0xFF,
  // clang-format on
};

Image Image::error_placeholder()
{
  Image out = {};
  out.m_dimensions = {2, 2};
  out.m_data = error_placeholder_data;
  return out;
}
