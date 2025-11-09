#include "image.hpp"

#include <fstream>

#include "engine.hpp"

namespace core
{

namespace image_impl
{

struct Chunk
{
  std::uint8_t* data;
  std::uint32_t length;
  std::uint32_t type;
};

static constexpr std::uint32_t IHDR = ('I' << 24) | ('H' << 16) | ('D' << 8) | ('R');
static constexpr std::uint32_t PLTE = ('P' << 24) | ('L' << 16) | ('T' << 8) | ('E');
static constexpr std::uint32_t IDAT = ('I' << 24) | ('D' << 16) | ('A' << 8) | ('T');
static constexpr std::uint32_t IEND = ('I' << 24) | ('E' << 16) | ('N' << 8) | ('D');
static constexpr std::uint32_t MAX_IDAT_CHUNK_SIZE = 1u << 30;

static constexpr std::array<std::uint8_t, 7>
  color_format_to_number_of_channels{1, static_cast<std::uint8_t>(-1), 3, 1, 2, static_cast<std::uint8_t>(4)};

inline static std::uint16_t bitReverse16(std::uint16_t n)
{
  n = ((n & 0xAAAA) >> 1) | static_cast<std::uint16_t>((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >> 2) | static_cast<std::uint16_t>((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >> 4) | static_cast<std::uint16_t>((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >> 8) | static_cast<std::uint16_t>((n & 0x00FF) << 8);
  return n;
}

inline static std::uint16_t bitReverse(std::uint16_t v, std::uint8_t bits)
{
  ASSERT(bits <= 16, "this function doesnt reverse more than 16 bits");
  return bitReverse16(v) >> (16 - bits);
}

namespace zlib
{

static constexpr std::uint8_t FAST_BITS = 9;
static constexpr std::uint16_t FAST_MASK = (1 << FAST_BITS) - 1;
static constexpr std::uint16_t NUM_SYMBOLS_LITERAL_LENGTH = 288;

struct Context;
struct Huffman
{
  std::uint16_t value[NUM_SYMBOLS_LITERAL_LENGTH];
  std::uint8_t length[NUM_SYMBOLS_LITERAL_LENGTH];
  std::uint32_t max_code[17];
  std::uint16_t first_code[16];
  std::uint16_t first_symbol[16];
  std::uint16_t fast_table[1 << FAST_BITS];

  void build(const std::uint8_t* code_lengths, std::size_t code_lengths_size);
  std::uint32_t decode(Context& zlib_ctx);
};

static constexpr std::uint8_t fixed_huffman_length_alphabet[NUM_SYMBOLS_LITERAL_LENGTH] = {
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8,
};

static constexpr std::uint8_t fixed_huffman_distance_alphabet[32] = {
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
};

struct Context
{
  Huffman length;
  Huffman distance;
  std::vector<std::uint8_t> out;
  std::uint8_t* data;
  std::uint8_t* data_end;
  std::uint32_t bits_buffered;
  std::uint32_t bits_buffer;
  bool hit_eof_once;

  bool dataInBounds()
  {
    return data < data_end;
  }

  std::uint8_t get8()
  {
    if (dataInBounds())
    {
      return *data++;
    }
    return static_cast<std::uint8_t>(-1);
  }

  void fillBits()
  {
    do
    {
      if (bits_buffer >= (1u << bits_buffered))
      {
        data = data_end;
        return;
      }
      bits_buffer |= static_cast<std::uint32_t>(get8() << bits_buffered);
      bits_buffered += 8;
    }
    while (bits_buffered <= 24);
  }

  std::uint32_t readBits(std::uint32_t n)
  {
    std::uint32_t k{};
    if (bits_buffered < n)
    {
      fillBits();
    }
    k = bits_buffer & static_cast<std::uint32_t>((1 << n) - 1);
    bits_buffer >>= n;
    bits_buffered -= n;
    return k;
  }

  bool eof()
  {
    return data >= data_end;
  }

  static constexpr std::uint16_t length_code_to_base_length[31] = {
    3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0,
  };
  static constexpr std::uint8_t length_code_to_extra_bits[31] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                                                                 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0};

  static constexpr std::uint16_t distance_code_to_base_length[32] = {
    1,   2,   3,   4,   5,    7,    9,    13,   17,   25,   33,   49,    65,    97,    129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0,   0,
  };
  static constexpr std::uint8_t distance_code_to_extra_bits[32] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                                                   6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

  void computeCodes()
  {
    static constexpr std::uint8_t code_length_indices[19] =
      {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    std::uint32_t hlit = readBits(5) + 257;
    std::uint32_t hdist = readBits(5) + 1;
    std::uint32_t hclen = readBits(4) + 4;
    std::uint32_t total_iterations = hlit + hdist;

    std::uint8_t code_length_sizes[19]{};
    Huffman code_length_tree{};
    for (std::uint32_t i = 0; i < hclen; ++i)
    {
      std::uint8_t code_length = static_cast<std::uint8_t>(readBits(3));
      code_length_sizes[code_length_indices[i]] = code_length;
    }
    code_length_tree.build(code_length_sizes, 19);

    std::uint32_t iteration_idx{};
    std::uint8_t code_lengths[286 + 32 + 137]{};
    while (iteration_idx < total_iterations)
    {
      std::uint32_t decoded_value = code_length_tree.decode(*this);
      if (decoded_value > 18)
      {
        throw std::runtime_error{"[PNG] bad code length"};
      }
      if (decoded_value < 16)
      {
        code_lengths[iteration_idx++] = static_cast<std::uint8_t>(decoded_value);
      }
      else
      {
        std::uint8_t fill = 0;
        std::uint32_t repeat_times;
        if (decoded_value == 16)
        {
          repeat_times = readBits(2) + 3;
          if (iteration_idx == 0)
          {
            throw std::runtime_error{"[PNG] bad code length"};
          }
          fill = code_lengths[iteration_idx - 1];
        }
        else if (decoded_value == 17)
        {
          repeat_times = readBits(3) + 3;
        }
        else
        {
          repeat_times = readBits(7) + 11;
        }
        if (total_iterations - iteration_idx < repeat_times)
        {
          throw std::runtime_error{"[PNG] bad code lengths"};
        }
        std::ranges::fill(code_lengths + iteration_idx, code_lengths + iteration_idx + repeat_times, fill);
        iteration_idx += repeat_times;
      }
    }
    if (iteration_idx != total_iterations)
    {
      throw std::runtime_error{"[PNG] bad code length"};
    }
    length.build(code_lengths, hlit);
    distance.build(code_lengths + hlit, hdist);
  }

  void parseBlock()
  {
    while (true)
    {
      std::uint32_t decoded_value = length.decode(*this);
      if (decoded_value == 256)
      {
        if (hit_eof_once && bits_buffered >= 16)
        {
          throw std::runtime_error{"[PNG] unexpected end"};
        }
        return;
      }
      else if (decoded_value < 256)
      {
        out.push_back(static_cast<std::uint8_t>(decoded_value));
      }
      else
      {
        if (decoded_value > 285)
        {
          throw std::runtime_error{"[PNG] bad huffman code"};
        }
        decoded_value -= 257;
        std::uint32_t len = length_code_to_base_length[decoded_value];
        if (length_code_to_extra_bits[decoded_value])
        {
          len += readBits(length_code_to_extra_bits[decoded_value]);
        }

        decoded_value = distance.decode(*this);
        if (decoded_value > 29)
        {
          throw std::runtime_error{"[PNG] bad huffman code"};
        }
        std::uint32_t dist = distance_code_to_base_length[decoded_value];
        if (distance_code_to_extra_bits[decoded_value])
        {
          dist += readBits(distance_code_to_extra_bits[decoded_value]);
        }
        if (out.size() < dist)
        {
          throw std::runtime_error{"[PNG] bad distance"};
        }

        std::size_t p_idx = out.size() - dist;
        while (len--)
        {
          out.push_back(out[p_idx++]);
        }
      }
    }
  }
};

void Huffman::build(const std::uint8_t* code_lengths, std::size_t code_lengths_size)
{
  std::uint32_t sizes[17] = {0};
  for (std::uint32_t i = 0; i < code_lengths_size; ++i)
  {
    ++sizes[code_lengths[i]];
  }
  sizes[0] = 0;
  for (std::uint32_t i = 1; i < 16; ++i)
  {
    if (sizes[i] > static_cast<std::uint32_t>(1 << i))
    {
      throw std::runtime_error{"[PNG] bad sizes"};
    }
  }

  std::uint32_t k = 0;
  std::uint32_t code = 0;
  std::uint32_t next_code[16] = {0};
  for (std::uint32_t i = 1; i < 16; ++i)
  {
    next_code[i] = code;
    first_code[i] = static_cast<std::uint16_t>(code);
    first_symbol[i] = static_cast<std::uint16_t>(k);
    code += sizes[i];
    if (sizes[i])
    {
      if (code - 1 >= static_cast<std::uint32_t>(1 << i))
      {
        throw std::runtime_error{"[PNG] bad code lengths"};
      }
    }
    max_code[i] = code << (16 - i);
    code <<= 1;
    k += sizes[i];
  }
  max_code[16] = 0x10000;

  std::ranges::fill(fast_table, 0);
  for (std::uint32_t i = 0; i < code_lengths_size; ++i)
  {
    std::uint8_t code_length = code_lengths[i];
    if (!code_length)
    {
      continue;
    }

    std::uint32_t canonical_pos = next_code[code_length] - first_code[code_length] + first_symbol[code_length];
    std::uint16_t fast_table_val = static_cast<std::uint16_t>(static_cast<std::uint32_t>(code_length << FAST_BITS) | i);
    length[canonical_pos] = static_cast<std::uint8_t>(code_length);
    value[canonical_pos] = static_cast<std::uint16_t>(i);
    if (code_length <= FAST_BITS)
    {
      std::uint32_t j = bitReverse(static_cast<std::uint16_t>(next_code[code_length]), code_length);
      while (j < (1 << FAST_BITS))
      {
        fast_table[j] = fast_table_val;
        j += (1 << code_length);
      }
    }
    ++next_code[code_length];
  }
}

std::uint32_t Huffman::decode(Context& zlib_ctx)
{
  if (zlib_ctx.bits_buffered < 16)
  {
    if (zlib_ctx.eof())
    {
      if (zlib_ctx.hit_eof_once)
      {
        throw std::runtime_error{"[PNG] corrupt zlib"};
      }
      zlib_ctx.hit_eof_once = true;
      zlib_ctx.bits_buffered += 16;
    }
    else
    {
      zlib_ctx.fillBits();
    }
  }

  // NOTE(szulf): trying to decode it through the fast lookup table
  std::uint16_t fast_value = fast_table[zlib_ctx.bits_buffer & FAST_MASK];
  if (fast_value)
  {
    std::uint16_t code_length = fast_value >> 9;
    zlib_ctx.bits_buffer >>= code_length;
    zlib_ctx.bits_buffered -= code_length;
    return fast_value & 511;
  }

  // NOTE(szulf): lookup table failed, doing it the slow way
  std::uint32_t bits = bitReverse(static_cast<std::uint16_t>(zlib_ctx.bits_buffer), 16);
  std::uint32_t code_length = FAST_BITS + 1;
  while (true)
  {
    if (bits < max_code[code_length])
    {
      break;
    }
    ++code_length;
  }
  if (code_length >= 16)
  {
    throw std::runtime_error{"[PNG] corrupt zlib"};
  }

  std::uint32_t value_idx = (bits >> (16 - code_length)) - first_code[code_length] + first_symbol[code_length];
  if (value_idx >= NUM_SYMBOLS_LITERAL_LENGTH)
  {
    throw std::runtime_error{"[PNG] corrupt zlib"};
  }
  if (length[value_idx] != code_length)
  {
    throw std::runtime_error{"[PNG] corrupt zlib"};
  }
  zlib_ctx.bits_buffer >>= code_length;
  zlib_ctx.bits_buffered -= code_length;
  return value[value_idx];
}

}

enum class FilterMethod : std::uint8_t
{
  None,
  Sub,
  Up,
  Average,
  Paeth,
  AverageFirstLine,
};

static constexpr FilterMethod first_line_filter_method[5] = {
  FilterMethod::None,
  FilterMethod::Sub,
  FilterMethod::None,
  FilterMethod::AverageFirstLine,
  FilterMethod::Sub,
};

static constexpr std::uint8_t RGBA_CHANNELS = 4;
static constexpr std::uint8_t RGBA8_BYTES_PER_PIXEL = 4;

static std::int32_t paethPredictor(std::int32_t a, std::int32_t b, std::int32_t c)
{
  std::int32_t p = a + b - c;
  std::int32_t pa = std::abs(p - a);
  std::int32_t pb = std::abs(p - b);
  std::int32_t pc = std::abs(p - c);
  if (pa <= pb && pa <= pc)
  {
    return a;
  }
  else if (pb <= pc)
  {
    return b;
  }
  else
  {
    return c;
  }
}

enum class CompressionType : std::uint8_t
{
  Uncompressed = 0,
  FixedHuffman = 1,
  DynamicHuffman = 2,
  Illegal = 3,
};

}

Image::Image(const std::filesystem::path& path)
{
  std::ifstream file_stream{path, std::ios::binary};
  std::stringstream ss{};
  ss << file_stream.rdbuf();
  std::string file{ss.str()};
  Context ctx{};
  ctx.data = reinterpret_cast<std::uint8_t*>(file.data());
  ctx.data_end = reinterpret_cast<std::uint8_t*>(file.data()) + file.size();

  // NOTE(szulf): check png header
  static const std::uint8_t png_header[] = {137, 80, 78, 71, 13, 10, 26, 10};
  for (std::uint32_t i = 0; i < 8; ++i)
  {
    if (ctx.get8() != png_header[i])
    {
      throw std::runtime_error{"[PNG] invalid header"};
    }
  }

  bool first = true;
  bool running = true;
  std::vector<std::uint8_t> combined_idat_chunks{};
  while (running && ctx.data != ctx.data_end)
  {
    image_impl::Chunk chunk = {};
    chunk.length = ctx.get32BE();
    chunk.type = ctx.get32BE();

    switch (chunk.type)
    {
      case image_impl::IHDR:
      {
        if (!first)
        {
          throw std::runtime_error{"ihdr not first"};
        }
        first = false;
        if (chunk.length != 13)
        {
          throw std::runtime_error{"invalid ihdr"};
        }
        m_width = ctx.get32BE();
        m_height = ctx.get32BE();
        if (!(m_width < MAX_SIZE && m_width > 0 && m_height < MAX_SIZE && m_height > 0))
        {
          throw std::runtime_error{"invalid ihdr"};
        }

        std::uint8_t bit_depth = ctx.get8();
        if (bit_depth > 16)
        {
          throw std::runtime_error{"invalid ihdr"};
        }
        ctx.bit_depth = bit_depth;
        std::uint8_t color = ctx.get8();
        if (color != 0 && color != 2 && color != 3 && color != 4 && color != 6)
        {
          throw std::runtime_error{"[PNG] invalid ihdr"};
        }
        ctx.color_format = static_cast<ColorFormat>(color);
        std::uint8_t compression = ctx.get8();
        if (compression != 0)
        {
          throw std::runtime_error{"[PNG] invalid ihdr"};
        }
        std::uint8_t filter = ctx.get8();
        if (filter != 0)
        {
          throw std::runtime_error{"[PNG] invalid ihdr"};
        }
        ctx.interlaced = ctx.get8();
      }
      break;

      case image_impl::PLTE:
      {
        ASSERT(false, "[TODO] support palettes");
      }
      break;

      case image_impl::IDAT:
      {
        if (first)
        {
          throw std::runtime_error{"[PNG] ihdr chunk not first"};
        }
        if (chunk.length > image_impl::MAX_IDAT_CHUNK_SIZE)
        {
          throw std::runtime_error{"[PNG] invalid idat"};
        }
        combined_idat_chunks.insert(combined_idat_chunks.end(), ctx.data, ctx.data + chunk.length);
        ctx.data += chunk.length;
      }
      break;

      case image_impl::IEND:
      {
        if (first)
        {
          throw std::runtime_error{"[PNG] ihdr chunk not first"};
        }
        if (combined_idat_chunks.size() == 0)
        {
          throw std::runtime_error{"[PNG] invalid idat"};
        }

        // NOTE(szulf): zlib parsing
        image_impl::zlib::Context zlib_ctx = {};
        zlib_ctx.data = combined_idat_chunks.data();
        zlib_ctx.data_end = combined_idat_chunks.data() + combined_idat_chunks.size();
        { // NOTE(szulf): header
          std::uint8_t cmf = zlib_ctx.get8();
          std::uint8_t cm = cmf & 15;
          std::uint8_t flg = zlib_ctx.get8();
          if (!zlib_ctx.dataInBounds() || static_cast<std::uint16_t>(cmf * 256 + flg) % 31 != 0 || flg & 32 || cm != 8)
          {
            throw std::runtime_error{"[PNG] corrupt zlib"};
          }
        }

        // NOTE(szulf): data
        zlib_ctx.bits_buffered = 0;
        zlib_ctx.bits_buffer = 0;
        bool final = false;
        image_impl::CompressionType type;
        do
        {
          final = zlib_ctx.readBits(1);
          type = static_cast<image_impl::CompressionType>(zlib_ctx.readBits(2));
          switch (type)
          {
            case image_impl::CompressionType::Uncompressed:
            {
              if (zlib_ctx.bits_buffered & 7)
              {
                zlib_ctx.readBits(zlib_ctx.bits_buffered & 7);
              }
              std::uint8_t header[4];
              std::uint32_t k = 0;
              while (zlib_ctx.bits_buffered > 0)
              {
                header[k++] = static_cast<std::uint8_t>(zlib_ctx.bits_buffer & 255);
                zlib_ctx.bits_buffer >>= 8;
                zlib_ctx.bits_buffered -= 8;
              }

              while (k < 4)
              {
                header[k++] = zlib_ctx.get8();
              }
              std::uint16_t len = static_cast<std::uint16_t>(header[1] << 8) | header[0];
              std::uint16_t nlen = static_cast<std::uint16_t>(header[3] << 8) | header[2];
              if (nlen != (len ^ 0xFFFF))
              {
                throw std::runtime_error{"[PNG] corrupt zlib"};
              }
              if (zlib_ctx.data + len > zlib_ctx.data_end)
              {
                throw std::runtime_error{"[PNG] read past buffer"};
              }
              std::ranges::copy(zlib_ctx.data, zlib_ctx.data + len, zlib_ctx.out.data() + zlib_ctx.out.size());
              zlib_ctx.data += len;
            }
            break;

            case image_impl::CompressionType::FixedHuffman:
            {
              zlib_ctx.length.build(
                static_cast<const std::uint8_t*>(image_impl::zlib::fixed_huffman_length_alphabet),
                image_impl::zlib::NUM_SYMBOLS_LITERAL_LENGTH
              );
              zlib_ctx.distance.build(image_impl::zlib::fixed_huffman_distance_alphabet, 32);
              zlib_ctx.parseBlock();
            }
            break;

            case image_impl::CompressionType::DynamicHuffman:
            {
              zlib_ctx.computeCodes();
              zlib_ctx.parseBlock();
            }
            break;

            case image_impl::CompressionType::Illegal:
            {
              throw std::runtime_error{"[PNG] illegal compression type"};
            }
            break;
          }
        }
        while (!final);

        if (!ctx.interlaced)
        {
          createRGBA8(ctx, zlib_ctx.out.data());
        }
        else
        {
          ASSERT(false, "[TODO] support interlaced pngs");
        }

        ctx.get32BE();
        running = false;
      }
      break;

      default:
      {
        if (first)
        {
          throw std::runtime_error{"[PNG] ihdr not first"};
        }
        ctx.data += chunk.length;
      }
      break;
    }
    // NOTE(szulf): skip CRC
    ctx.get32BE();
  }
}

Image::~Image()
{
  delete[] m_data;
  m_data = nullptr;
}

std::uint8_t Image::Context::get8()
{
  if (data < data_end)
  {
    return *data++;
  }
  return static_cast<std::uint8_t>(-1);
}

std::uint16_t Image::Context::get16BE()
{
  std::uint16_t v = get8();
  return static_cast<std::uint16_t>((v << 8) + get8());
}

std::uint32_t Image::Context::get32BE()
{
  std::uint32_t v = get16BE();
  return (v << 16) + get16BE();
}

void Image::createRGBA8(Context& ctx, std::uint8_t* data)
{
  std::uint8_t channels = image_impl::color_format_to_number_of_channels[static_cast<std::size_t>(ctx.color_format)];
  std::uint8_t byte_depth = ctx.bit_depth == 16 ? 2 : 1;
  std::uint32_t data_bytes_per_pixel = byte_depth * channels;
  std::size_t stride = m_width * image_impl::RGBA8_BYTES_PER_PIXEL;

  m_data = new std::uint8_t[m_width * m_height * image_impl::RGBA8_BYTES_PER_PIXEL];

  // NOTE(szulf): (x + 0b111) >> 3 is basically doing ceil(x / 8.0f)
  std::size_t bytes_per_scanline = ((m_width * channels * ctx.bit_depth) + 7) >> 3;
  std::uint8_t* filter_buffer = new std::uint8_t[bytes_per_scanline * 2];

  std::size_t width = m_width;
  if (ctx.bit_depth < 8)
  {
    data_bytes_per_pixel = 1;
    width = bytes_per_scanline;
  }

  std::uint8_t* curr = filter_buffer;
  std::uint8_t* prev = filter_buffer + bytes_per_scanline;
  for (std::uint32_t line_idx = 0; line_idx < m_height; ++line_idx)
  {
    std::uint8_t filter_method_val = *data++;
    if (filter_method_val > 4)
    {
      throw std::runtime_error{"[PNG] invalid filter"};
    }
    if (line_idx == 0)
    {
      filter_method_val = static_cast<std::uint8_t>(image_impl::first_line_filter_method[filter_method_val]);
    }
    std::size_t data_bytes_per_line = data_bytes_per_pixel * width;
    std::uint8_t* dest = reinterpret_cast<std::uint8_t*>(m_data) + (stride * line_idx);

    image_impl::FilterMethod filter_method = static_cast<image_impl::FilterMethod>(filter_method_val);
    switch (filter_method)
    {
      case image_impl::FilterMethod::None:
      {
        std::ranges::copy_n(data, static_cast<std::int64_t>(data_bytes_per_line), curr);
      }
      break;
      case image_impl::FilterMethod::Sub:
      {
        std::ranges::copy_n(data, static_cast<std::int64_t>(data_bytes_per_pixel), curr);
        for (std::uint32_t i = data_bytes_per_pixel; i < data_bytes_per_line; ++i)
        {
          curr[i] = (data[i] + curr[i - data_bytes_per_pixel]) & 255;
        }
      }
      break;
      case image_impl::FilterMethod::Up:
      {
        for (std::uint32_t i = 0; i < data_bytes_per_line; ++i)
        {
          curr[i] = (data[i] + prev[i]) & 255;
        }
      }
      break;
      case image_impl::FilterMethod::Average:
      {
        for (std::uint32_t i = 0; i < data_bytes_per_pixel; ++i)
        {
          curr[i] = (data[i] + (prev[i] / 2)) & 255;
        }
        for (std::uint32_t i = data_bytes_per_pixel; i < data_bytes_per_line; ++i)
        {
          curr[i] = ((data[i] + ((prev[i] + curr[i - data_bytes_per_pixel]) / 2)) & 255);
        }
      }
      break;
      case image_impl::FilterMethod::Paeth:
      {
        for (std::uint32_t i = 0; i < data_bytes_per_pixel; ++i)
        {
          curr[i] = (data[i] + prev[i]) & 255;
        }
        for (std::uint32_t i = data_bytes_per_pixel; i < data_bytes_per_line; ++i)
        {
          curr[i] =
            (data[i] +
             image_impl::paethPredictor(curr[i - data_bytes_per_pixel], prev[i], prev[i - data_bytes_per_pixel])) &
            255;
        }
      }
      break;
      case image_impl::FilterMethod::AverageFirstLine:
      {
        std::ranges::copy_n(data, data_bytes_per_pixel, curr);
        for (std::uint32_t i = data_bytes_per_pixel; i < data_bytes_per_line; ++i)
        {
          curr[i] = (data[i] + curr[i - data_bytes_per_pixel]) & 255;
        }
      }
      break;
    }
    data += data_bytes_per_line;

    if (ctx.bit_depth == 8)
    {
      if (channels == image_impl::RGBA_CHANNELS)
      {
        std::ranges::copy_n(curr, static_cast<std::int64_t>(width * image_impl::RGBA_CHANNELS), dest);
      }
      else if (channels == 1)
      {
        ASSERT(false, "[TODO] support grayscale");
      }
      else
      {
        ASSERT(channels == 3, "png only allows 1, 3, 4 channels");
        for (std::uint32_t i = 0; i < m_width; ++i)
        {
          dest[i * 4 + 0] = curr[i * 3 + 0];
          dest[i * 4 + 1] = curr[i * 3 + 1];
          dest[i * 4 + 2] = curr[i * 3 + 2];
          dest[i * 4 + 3] = 255;
        }
      }
    }
    else if (ctx.bit_depth < 8)
    {
      ASSERT(false, "[TODO] support smaller depths");
    }
    else if (ctx.bit_depth == 16)
    {
      ASSERT(false, "[TODO] support 16 bit depth")
    }

    std::swap(curr, prev);
  }

  delete[] filter_buffer;
}

}
