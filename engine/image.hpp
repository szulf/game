#pragma once

#include <filesystem>

namespace core
{

static constexpr std::uint32_t MAX_SIZE = 1 << 24;

class Image
{
public:
  // TODO(szulf): on fail dont throw exceptions initialize with an error image
  Image(const std::filesystem::path& path);
  ~Image();

  inline auto width() const -> std::size_t
  {
    return m_width;
  }
  inline auto height() const -> std::size_t
  {
    return m_height;
  }

private:
  // TODO(szulf): does this have to live here???
  // createRGBA8 needs to set private members of the Image class,
  // and even if i wanted to make it a friend i would need to define what Context is
  enum class ColorFormat : std::uint8_t
  {
    Grayscale = 0,
    Rgb = 2,
    Palette = 3,
    GrayscaleAlpha = 4,
    Rgba = 6,
  };

  struct Context
  {
    std::uint8_t* data;
    std::uint8_t* data_end;
    std::uint8_t bit_depth;
    bool interlaced;
    ColorFormat color_format;

    std::uint8_t get8();
    std::uint16_t get16BE();
    std::uint32_t get32BE();
  };

  void createRGBA8(Context& ctx, std::uint8_t* data);

private:
  std::uint8_t* m_data{};
  std::size_t m_width{};
  std::size_t m_height{};
};

}
