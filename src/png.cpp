#include "png.h"

namespace png
{

Image::Image(Image&& other)
  : data{other.data}, size{other.size}, width{other.width}, height{other.height},
    channels{other.channels}
{
  other.data = nullptr;
  other.size = 0;
  other.width = 0;
  other.height = 0;
  other.channels = 0;
}

Image& Image::operator=(Image&& other)
{
  data = other.data;
  size = other.size;
  width = other.width;
  height = other.height;
  channels = other.channels;
  other.data = nullptr;
  other.size = 0;
  other.width = 0;
  other.height = 0;
  other.channels = 0;
  return *this;
}

Image::~Image()
{
  delete[] data;
  data = nullptr;
}

constexpr u32 MAX_IDAT_CHUNK_SIZE = 1u << 30;

enum class ColorFormat : u8
{
  Grayscale,
  Rgb,
  Palette,
  GrayscaleAlpha,
  Rgba,
};

u32 color_to_color_format[7] = {
  (u32) ColorFormat::Grayscale,
  (u32) -1,
  (u32) ColorFormat::Rgb,
  (u32) ColorFormat::Palette,
  (u32) ColorFormat::GrayscaleAlpha,
  (u32) -1,
  (u32) ColorFormat::Rgba,
};

u8 color_format_to_number_of_channels[5] = {1, 3, 1, 2, 4};

struct Context
{
  u8* data;
  u8* data_end;
  bool interlaced;
  ColorFormat color_format;
  u8 bit_depth;
};

inline static u8 get8(Context& ctx)
{
  if (ctx.data < ctx.data_end)
  {
    return *ctx.data++;
  }
  return (u8) -1;
}

inline static u16 get16be(Context& ctx)
{
  u16 v = get8(ctx);
  return (u16) (v << 8) + get8(ctx);
}

inline static u32 get32be(Context& ctx)
{
  u32 v = get16be(ctx);
  return (v << 16) + get16be(ctx);
}

struct Chunk
{
  u8* data;
  u32 length;
  u32 type;
};

inline static u16 image_bit_reverse_16(u16 n)
{
  n = ((n & 0xAAAA) >> 1) | (u16) ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >> 2) | (u16) ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >> 4) | (u16) ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >> 8) | (u16) ((n & 0x00FF) << 8);
  return n;
}

inline static u16 image_bit_reverse(u16 v, u32 bits)
{
  ASSERT(bits <= 16, "this function doesnt reverse more than 16 bits");
  return image_bit_reverse_16(v) >> (16 - bits);
}

namespace zlib
{

constexpr u8 FAST_BITS = 9;
constexpr u16 FAST_MASK = (1 << FAST_BITS) - 1;
constexpr u16 NUM_SYMBOLS_LITERAL_LENGTH = 288;

struct Huffman
{
  u16 value[NUM_SYMBOLS_LITERAL_LENGTH];
  u8 length[NUM_SYMBOLS_LITERAL_LENGTH];
  u32 max_code[17];
  u16 first_code[16];
  u16 first_symbol[16];
  u16 fast_table[1 << FAST_BITS];

  void build(u8* code_lengths, usize code_lengths_size);
};

static const u8 fixed_huffman_length_alphabet[NUM_SYMBOLS_LITERAL_LENGTH] = {
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8,
};

static const u8 fixed_huffman_distance_alphabet[32] = {
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
};

struct Context
{
  bool hit_eof_once;
  u32 bits_buffered;
  u32 bits_buffer;
  u8* data;
  u8* data_end;
  std::vector<u8> out;
  Huffman length;
  Huffman distance;
};

inline static bool data_in_bounds(Context& zlib_ctx)
{
  return zlib_ctx.data < zlib_ctx.data_end;
}

inline static u8 get8(Context& zlib_ctx)
{
  if (data_in_bounds(zlib_ctx))
  {
    return *zlib_ctx.data++;
  }
  return (u8) -1;
}

static void fill_bits(Context& zlib_ctx)
{
  do
  {
    if (zlib_ctx.bits_buffer >= (1u << zlib_ctx.bits_buffered))
    {
      zlib_ctx.data = zlib_ctx.data_end;
      return;
    }
    zlib_ctx.bits_buffer |= (u32) (get8(zlib_ctx) << zlib_ctx.bits_buffered);
    zlib_ctx.bits_buffered += 8;
  }
  while (zlib_ctx.bits_buffered <= 24);
}

static u32 read_bits(Context& zlib_ctx, u32 n)
{
  u32 k = 0;
  if (zlib_ctx.bits_buffered < n)
  {
    fill_bits(zlib_ctx);
  }
  k = zlib_ctx.bits_buffer & (u32) ((1 << n) - 1);
  zlib_ctx.bits_buffer >>= n;
  zlib_ctx.bits_buffered -= n;
  return k;
}

void Huffman::build(u8* code_lengths, usize code_lengths_size)
{
  u32 sizes[17] = {0};
  for (u32 i = 0; i < code_lengths_size; ++i)
  {
    ++sizes[code_lengths[i]];
  }
  sizes[0] = 0;
  for (u32 i = 1; i < 16; ++i)
  {
    if (sizes[i] > static_cast<u32>(1 << i))
    {
      throw std::runtime_error{"[PNG] bad sizes"};
    }
  }

  u32 k = 0;
  u32 code = 0;
  u32 next_code[16] = {0};
  for (u32 i = 1; i < 16; ++i)
  {
    next_code[i] = code;
    first_code[i] = (u16) code;
    first_symbol[i] = (u16) k;
    code += sizes[i];
    if (sizes[i])
    {
      if (code - 1 >= static_cast<u32>(1 << i))
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
  for (u32 i = 0; i < code_lengths_size; ++i)
  {
    u8 code_length = code_lengths[i];
    if (!code_length)
    {
      continue;
    }

    u32 canonical_pos =
      next_code[code_length] - first_code[code_length] + first_symbol[code_length];
    u16 fast_table_val = (u16) ((u32) (code_length << FAST_BITS) | i);
    length[canonical_pos] = (u8) code_length;
    value[canonical_pos] = (u16) i;
    if (code_length <= FAST_BITS)
    {
      u32 j = image_bit_reverse((u16) next_code[code_length], code_length);
      while (j < (1 << FAST_BITS))
      {
        fast_table[j] = fast_table_val;
        j += (1 << code_length);
      }
    }
    ++next_code[code_length];
  }
}

inline static bool eof(Context& zlib_ctx)
{
  return zlib_ctx.data >= zlib_ctx.data_end;
}

static u32 huffman_decode(Context& zlib_ctx, Huffman& huffman)
{
  if (zlib_ctx.bits_buffered < 16)
  {
    if (eof(zlib_ctx))
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
      fill_bits(zlib_ctx);
    }
  }

  // NOTE(szulf): trying to decode it through the fast lookup table
  u16 fast_value = huffman.fast_table[zlib_ctx.bits_buffer & FAST_MASK];
  if (fast_value)
  {
    u16 code_length = fast_value >> 9;
    zlib_ctx.bits_buffer >>= code_length;
    zlib_ctx.bits_buffered -= code_length;
    return fast_value & 511;
  }

  // NOTE(szulf): lookup table failed, doing it the slow way
  u32 bits = image_bit_reverse((u16) zlib_ctx.bits_buffer, 16);
  u32 code_length = FAST_BITS + 1;
  while (true)
  {
    if (bits < huffman.max_code[code_length])
    {
      break;
    }
    ++code_length;
  }
  if (code_length >= 16)
  {
    throw std::runtime_error{"[PNG] corrupt zlib"};
  }

  u32 value_idx = (bits >> (16 - code_length)) - huffman.first_code[code_length] +
                  huffman.first_symbol[code_length];
  if (value_idx >= NUM_SYMBOLS_LITERAL_LENGTH)
  {
    throw std::runtime_error{"[PNG] corrupt zlib"};
  }
  if (huffman.length[value_idx] != code_length)
  {
    throw std::runtime_error{"[PNG] corrupt zlib"};
  }
  zlib_ctx.bits_buffer >>= code_length;
  zlib_ctx.bits_buffered -= code_length;
  return huffman.value[value_idx];
}

static void compute_codes(Context& zlib_ctx)
{
  static const u8 code_length_indices[19] =
    {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
  u32 hlit = read_bits(zlib_ctx, 5) + 257;
  u32 hdist = read_bits(zlib_ctx, 5) + 1;
  u32 hclen = read_bits(zlib_ctx, 4) + 4;
  u32 total_iterations = hlit + hdist;

  u8 code_length_sizes[19] = {};
  Huffman code_length_tree = {};
  for (u32 i = 0; i < hclen; ++i)
  {
    u8 code_length = (u8) read_bits(zlib_ctx, 3);
    code_length_sizes[code_length_indices[i]] = code_length;
  }
  code_length_tree.build(code_length_sizes, 19);

  u32 iteration_idx = 0;
  u8 code_lengths[286 + 32 + 137] = {0};
  while (iteration_idx < total_iterations)
  {
    u32 decoded_value = huffman_decode(zlib_ctx, code_length_tree);
    if (decoded_value > 18)
    {
      throw std::runtime_error{"[PNG] bad code length"};
    }
    if (decoded_value < 16)
    {
      code_lengths[iteration_idx++] = (u8) decoded_value;
    }
    else
    {
      u8 fill = 0;
      u32 repeat_times;
      if (decoded_value == 16)
      {
        repeat_times = read_bits(zlib_ctx, 2) + 3;
        if (iteration_idx == 0)
        {
          throw std::runtime_error{"[PNG] bad code length"};
        }
        fill = code_lengths[iteration_idx - 1];
      }
      else if (decoded_value == 17)
      {
        repeat_times = read_bits(zlib_ctx, 3) + 3;
      }
      else
      {
        repeat_times = read_bits(zlib_ctx, 7) + 11;
      }
      if (total_iterations - iteration_idx < repeat_times)
      {
        throw std::runtime_error{"[PNG] bad code lengths"};
      }
      std::ranges::fill(
        code_lengths + iteration_idx,
        code_lengths + iteration_idx + repeat_times,
        fill
      );
      iteration_idx += repeat_times;
    }
  }
  if (iteration_idx != total_iterations)
  {
    throw std::runtime_error{"[PNG] bad code length"};
  }
  zlib_ctx.length.build(code_lengths, hlit);
  zlib_ctx.distance.build(code_lengths + hlit, hdist);
}

static const u16 length_code_to_base_length[31] = {
  3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27, 31,
  35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0,
};
static const u8 length_code_to_extra_bits[31] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                                                 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0};

static const u16 distance_code_to_base_length[32] = {
  1,   2,   3,   4,   5,    7,    9,    13,   17,   25,   33,   49,    65,    97,    129, 193,
  257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0,   0,
};
static const u8 distance_code_to_extra_bits[32] = {0, 0, 0,  0,  1,  1,  2,  2,  3,  3,
                                                   4, 4, 5,  5,  6,  6,  7,  7,  8,  8,
                                                   9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

static void huffman_parse_block(Context& zlib_ctx)
{
  while (true)
  {
    u32 decoded_value = huffman_decode(zlib_ctx, zlib_ctx.length);
    if (decoded_value == 256)
    {
      if (zlib_ctx.hit_eof_once && zlib_ctx.bits_buffered >= 16)
      {
        throw std::runtime_error{"[PNG] unexpected end"};
      }
      return;
    }
    else if (decoded_value < 256)
    {
      zlib_ctx.out.push_back(static_cast<u8>(decoded_value));
    }
    else
    {
      if (decoded_value > 285)
      {
        throw std::runtime_error{"[PNG] bad huffman code"};
      }
      decoded_value -= 257;
      u32 len = length_code_to_base_length[decoded_value];
      if (length_code_to_extra_bits[decoded_value])
      {
        len += read_bits(zlib_ctx, length_code_to_extra_bits[decoded_value]);
      }

      decoded_value = huffman_decode(zlib_ctx, zlib_ctx.distance);
      if (decoded_value > 29)
      {
        throw std::runtime_error{"[PNG] bad huffman code"};
      }
      u32 dist = distance_code_to_base_length[decoded_value];
      if (distance_code_to_extra_bits[decoded_value])
      {
        dist += read_bits(zlib_ctx, distance_code_to_extra_bits[decoded_value]);
      }
      if (zlib_ctx.out.size() < dist)
      {
        throw std::runtime_error{"[PNG] bad distance"};
      }

      usize p_idx = zlib_ctx.out.size() - dist;
      while (len--)
      {
        zlib_ctx.out.push_back(zlib_ctx.out[p_idx++]);
      }
    }
  }
}

}

enum class FilterMethod : u8
{
  None,
  Sub,
  Up,
  Average,
  Paeth,
  AverageFirstLine,
};

u8 first_line_filter_method[5] = {
  (u8) FilterMethod::None,
  (u8) FilterMethod::Sub,
  (u8) FilterMethod::None,
  (u8) FilterMethod::AverageFirstLine,
  (u8) FilterMethod::Sub,
};

constexpr u8 RGBA_CHANNELS = 4;
constexpr u8 RGBA8_BYTES_PER_PIXEL = 4;

static i32 image_png_paeth_predictor(i32 a, i32 b, i32 c)
{
  i32 p = a + b - c;
  i32 pa = std::abs(p - a);
  i32 pb = std::abs(p - b);
  i32 pc = std::abs(p - c);
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

static void create_rgba8(Image& img, Context& ctx, u8* data)
{
  u8 channels = color_format_to_number_of_channels[(usize) ctx.color_format];
  u8 byte_depth = ctx.bit_depth == 16 ? 2 : 1;
  u32 data_bytes_per_pixel = byte_depth * channels;
  usize stride = img.width * RGBA8_BYTES_PER_PIXEL;

  img.data = new u8[img.width * img.height * RGBA8_BYTES_PER_PIXEL];

  // NOTE(szulf): (x + 0b111) >> 3 is basically doing ceil(x / 8.0f)
  usize bytes_per_scanline = ((img.width * channels * ctx.bit_depth) + 7) >> 3;
  u8* filter_buffer = new u8[bytes_per_scanline * 2];

  usize width = img.width;
  if (ctx.bit_depth < 8)
  {
    data_bytes_per_pixel = 1;
    width = bytes_per_scanline;
  }

  u8* curr = filter_buffer;
  u8* prev = filter_buffer + bytes_per_scanline;
  for (u32 line_idx = 0; line_idx < img.height; ++line_idx)
  {
    u8 filter_method_val = *data++;
    if (filter_method_val > 4)
    {
      throw std::runtime_error{"[PNG] invalid filter"};
    }
    if (line_idx == 0)
    {
      filter_method_val = first_line_filter_method[filter_method_val];
    }
    usize data_bytes_per_line = data_bytes_per_pixel * width;
    u8* dest = ((u8*) img.data) + (stride * line_idx);

    FilterMethod filter_method = (FilterMethod) filter_method_val;
    switch (filter_method)
    {
      case FilterMethod::None:
      {
        std::ranges::copy_n(data, static_cast<i64>(data_bytes_per_line), curr);
      }
      break;
      case FilterMethod::Sub:
      {
        std::ranges::copy_n(data, static_cast<i64>(data_bytes_per_pixel), curr);
        for (u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i)
        {
          curr[i] = (data[i] + curr[i - data_bytes_per_pixel]) & 255;
        }
      }
      break;
      case FilterMethod::Up:
      {
        for (u32 i = 0; i < data_bytes_per_line; ++i)
        {
          curr[i] = (data[i] + prev[i]) & 255;
        }
      }
      break;
      case FilterMethod::Average:
      {
        for (u32 i = 0; i < data_bytes_per_pixel; ++i)
        {
          curr[i] = (data[i] + (prev[i] / 2)) & 255;
        }
        for (u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i)
        {
          curr[i] = (u8) ((data[i] + ((prev[i] + curr[i - data_bytes_per_pixel]) / 2)) & 255);
        }
      }
      break;
      case FilterMethod::Paeth:
      {
        for (u32 i = 0; i < data_bytes_per_pixel; ++i)
        {
          curr[i] = (data[i] + prev[i]) & 255;
        }
        for (u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i)
        {
          curr[i] = (u8) (data[i] + image_png_paeth_predictor(
                                      curr[i - data_bytes_per_pixel],
                                      prev[i],
                                      prev[i - data_bytes_per_pixel]
                                    )) &
                    255;
        }
      }
      break;
      case FilterMethod::AverageFirstLine:
      {
        std::ranges::copy_n(data, data_bytes_per_pixel, curr);
        for (u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i)
        {
          curr[i] = (data[i] + curr[i - data_bytes_per_pixel]) & 255;
        }
      }
      break;
    }
    data += data_bytes_per_line;

    if (ctx.bit_depth == 8)
    {
      if (channels == RGBA_CHANNELS)
      {
        std::ranges::copy_n(curr, static_cast<i64>(width * RGBA_CHANNELS), dest);
      }
      else if (channels == 1)
      {
        TODO("support grayscale");
      }
      else
      {
        ASSERT(channels == 3, "png only allows 1, 3, 4 channels");
        for (u32 i = 0; i < img.width; ++i)
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
      TODO("support smaller depths");
    }
    else if (ctx.bit_depth == 16)
    {
      TODO("support 16 bit depth");
    }

    u8* temp = curr;
    curr = prev;
    prev = temp;
  }

  delete[] filter_buffer;
}

enum class CompressionType : u8
{
  Uncompressed = 0,
  FixedHuffman = 1,
  DynamicHuffman = 2,
  Illegal = 3,
};

#define IMAGE_PNG_TYPE(a, b, c, d) (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))

Image Image::decode(const std::filesystem::path& path)
{
  Image img{};
  std::ifstream file_stream{path, std::ios::binary};
  std::stringstream ss{};
  ss << file_stream.rdbuf();
  std::string file{ss.str()};
  Context ctx{};
  ctx.data = reinterpret_cast<u8*>(file.data());
  ctx.data_end = reinterpret_cast<u8*>(file.data()) + file.size();

  // NOTE(szulf): check png header
  static const u8 png_header[] = {137, 80, 78, 71, 13, 10, 26, 10};
  for (u32 i = 0; i < 8; ++i)
  {
    if (get8(ctx) != png_header[i])
    {
      throw std::runtime_error{"[PNG] invalid header"};
    }
  }

  bool first = true;
  bool running = true;
  std::vector<u8> combined_idat_chunks{};
  while (running && ctx.data != ctx.data_end)
  {
    Chunk chunk = {};
    chunk.length = get32be(ctx);
    chunk.type = get32be(ctx);

    switch (chunk.type)
    {
      case IMAGE_PNG_TYPE('I', 'H', 'D', 'R'):
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
        img.width = get32be(ctx);
        img.height = get32be(ctx);
        if (!(img.width < MAX_SIZE && img.width > 0 && img.height < MAX_SIZE && img.height > 0))
        {
          throw std::runtime_error{"invalid ihdr"};
        }

        u8 bit_depth = get8(ctx);
        if (bit_depth < 0 || bit_depth > 16)
        {
          throw std::runtime_error{"invalid ihdr"};
        }
        ctx.bit_depth = bit_depth;
        u8 color = get8(ctx);
        if (color != 0 && color != 2 && color != 3 && color != 4 && color != 6)
        {
          throw std::runtime_error{"[PNG] invalid ihdr"};
        }
        ctx.color_format = static_cast<ColorFormat>(color_to_color_format[color]);
        u8 compression = get8(ctx);
        if (compression != 0)
        {
          throw std::runtime_error{"[PNG] invalid ihdr"};
        }
        u8 filter = get8(ctx);
        if (filter != 0)
        {
          throw std::runtime_error{"[PNG] invalid ihdr"};
        }
        ctx.interlaced = get8(ctx);
      }
      break;

      case IMAGE_PNG_TYPE('P', 'L', 'T', 'E'):
      {
        TODO("support palettes");
      }
      break;

      case IMAGE_PNG_TYPE('I', 'D', 'A', 'T'):
      {
        if (first)
        {
          throw std::runtime_error{"[PNG] ihdr chunk not first"};
        }
        if (chunk.length > MAX_IDAT_CHUNK_SIZE)
        {
          throw std::runtime_error{"[PNG] invalid idat"};
        }
        combined_idat_chunks.insert(combined_idat_chunks.end(), ctx.data, ctx.data + chunk.length);
        ctx.data += chunk.length;
      }
      break;

      case IMAGE_PNG_TYPE('I', 'E', 'N', 'D'):
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
        zlib::Context zlib_ctx = {};
        zlib_ctx.data = combined_idat_chunks.data();
        zlib_ctx.data_end = combined_idat_chunks.data() + combined_idat_chunks.size();
        { // NOTE(szulf): header
          u8 cmf = zlib::get8(zlib_ctx);
          u8 cm = cmf & 15;
          u8 flg = zlib::get8(zlib_ctx);
          if (!zlib::data_in_bounds(zlib_ctx) || static_cast<u16>(cmf * 256 + flg) % 31 != 0 ||
              flg & 32 || cm != 8)
          {
            throw std::runtime_error{"[PNG] corrupt zlib"};
          }
        }

        // NOTE(szulf): data
        zlib_ctx.bits_buffered = 0;
        zlib_ctx.bits_buffer = 0;
        bool final = false;
        CompressionType type;
        do
        {
          final = zlib::read_bits(zlib_ctx, 1);
          type = (CompressionType) zlib::read_bits(zlib_ctx, 2);
          switch (type)
          {
            case CompressionType::Uncompressed:
            {
              if (zlib_ctx.bits_buffered & 7)
              {
                zlib::read_bits(zlib_ctx, zlib_ctx.bits_buffered & 7);
              }
              u8 header[4];
              u32 k = 0;
              while (zlib_ctx.bits_buffered > 0)
              {
                header[k++] = (u8) (zlib_ctx.bits_buffer & 255);
                zlib_ctx.bits_buffer >>= 8;
                zlib_ctx.bits_buffered -= 8;
              }

              while (k < 4)
              {
                header[k++] = zlib::get8(zlib_ctx);
              }
              u16 len = (u16) (header[1] << 8) | header[0];
              u16 nlen = (u16) (header[3] << 8) | header[2];
              if (nlen != (len ^ 0xFFFF))
              {
                throw std::runtime_error{"[PNG] corrupt zlib"};
              }
              if (zlib_ctx.data + len > zlib_ctx.data_end)
              {
                throw std::runtime_error{"[PNG] read past buffer"};
              }
              std::ranges::copy(
                zlib_ctx.data,
                zlib_ctx.data + len,
                zlib_ctx.out.data() + zlib_ctx.out.size()
              );
              zlib_ctx.data += len;
            }
            break;

            case CompressionType::FixedHuffman:
            {
              zlib_ctx.length.build(
                (u8*) zlib::fixed_huffman_length_alphabet,
                zlib::NUM_SYMBOLS_LITERAL_LENGTH
              );
              zlib_ctx.distance.build((u8*) zlib::fixed_huffman_distance_alphabet, 32);
              zlib::huffman_parse_block(zlib_ctx);
            }
            break;

            case CompressionType::DynamicHuffman:
            {
              zlib::compute_codes(zlib_ctx);
              zlib::huffman_parse_block(zlib_ctx);
            }
            break;

            case CompressionType::Illegal:
            default:
            {
              throw std::runtime_error{"[PNG] illegal compression type"};
            }
            break;
          }
        }
        while (!final);

        if (!ctx.interlaced)
        {
          create_rgba8(img, ctx, zlib_ctx.out.data());
        }
        else
        {
          TODO("support interlaced pngs");
        }

        get32be(ctx);
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
    get32be(ctx);
  }

  return img;
}

}
