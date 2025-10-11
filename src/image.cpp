#include "image.h"

namespace image {

static constexpr std::uint32_t MAX_IDAT_CHUNK_SIZE = 1u << 30;

enum class PngColorFormat : u8 {
  Grayscale,
  Rgb,
  Palette,
  GrayscaleAlpha,
  Rgba,
  Invalid,
};

PngColorFormat color_to_color_format[7] = {
  PngColorFormat::Grayscale,
  PngColorFormat::Invalid,
  PngColorFormat::Rgb,
  PngColorFormat::Palette,
  PngColorFormat::GrayscaleAlpha,
  PngColorFormat::Invalid,
  PngColorFormat::Rgba,
};

u8 color_format_to_number_of_channels[5] = {1, 3, 1, 2, 4};

struct Context {
  std::vector<std::uint8_t> data;
  usize data_idx;
  bool interlaced;
  PngColorFormat color_format;
  u8 bit_depth;
};

inline static u8
get8(Context& ctx) {
  if (ctx.data_idx < ctx.data.size()) return ctx.data[ctx.data_idx++];
  return (u8) -1;
}

inline static u16
get16be(Context& ctx) {
  u16 v = get8(ctx);
  return (u16) (v << 8) + get8(ctx);
}

inline static u32
get32be(Context& ctx) {
  u32 v = get16be(ctx);
  return (v << 16) + get16be(ctx);
}

struct PngChunk {
  u8* data;
  u32 length;
  u32 type;
};

static constexpr std::uint8_t ZLIB_FAST_BITS = 9;
static constexpr std::uint16_t ZLIB_FAST_MASK = (1 << ZLIB_FAST_BITS) - 1;
static constexpr std::uint16_t ZLIB_NUM_SYMBOLS_LITERAL_LENGTH = 288;

struct Huffman {
  u16 value[ZLIB_NUM_SYMBOLS_LITERAL_LENGTH];
  u8  length[ZLIB_NUM_SYMBOLS_LITERAL_LENGTH];
  u32 max_code[17];
  u16 first_code[16];
  u16 first_symbol[16];
  u16 fast_table[1 << ZLIB_FAST_BITS];
};

static const u8 image_zlib_fixed_huffman_length_alphabet[ZLIB_NUM_SYMBOLS_LITERAL_LENGTH] = {
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

static const u8 image_zlib_fixed_huffman_distance_alphabet[32] = {
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
};

enum class PngCompressionType : u8 {
  Uncompressed = 0,
  FixedHuffman = 1,
  DynamicHuffman = 2,
  Illegal = 3,
};

struct ZlibContext {
  bool hit_eof_once;
  u32 bits_buffered;
  u32 bits_buffer;
  u8* data;
  u8* data_end;
  std::vector<u8> out;
  Huffman length;
  Huffman distance;
};

inline static bool
zlib_data_in_bounds(ZlibContext& zlib_ctx) {
  return zlib_ctx.data < zlib_ctx.data_end;
}

inline static u8
zlib_get8(ZlibContext& zlib_ctx) {
  if (zlib_data_in_bounds(zlib_ctx)) return *zlib_ctx.data++;
  return (u8) -1;
}

static void
zlib_fill_bits(ZlibContext& zlib_ctx) {
  do {
    if (zlib_ctx.bits_buffer >= (1u << zlib_ctx.bits_buffered)) {
      zlib_ctx.data = zlib_ctx.data_end;
      return;
    }
    zlib_ctx.bits_buffer |= (u32) (zlib_get8(zlib_ctx) << zlib_ctx.bits_buffered);
    zlib_ctx.bits_buffered += 8;
  } while (zlib_ctx.bits_buffered <= 24);
}

static u32
zlib_read_bits(ZlibContext& zlib_ctx, u32 n) {
  u32 k = 0;
  if (zlib_ctx.bits_buffered < n) zlib_fill_bits(zlib_ctx);
  k = zlib_ctx.bits_buffer & ((1 << n) - 1);
  zlib_ctx.bits_buffer >>= n;
  zlib_ctx.bits_buffered -= n;
  return k;
}

inline static u16
bit_reverse_16(u16 n) {
  n = ((n & 0xAAAA) >>  1) | (u16) ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | (u16) ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | (u16) ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | (u16) ((n & 0x00FF) << 8);
  return n;
}

inline static u16
bit_reverse(u16 v, u32 bits) {
  ASSERT(bits <= 16, "this function doesnt reverse more than 16 bits");
  return bit_reverse_16(v) >> (16 - bits);
}

static void
zlib_huffman_build(Huffman& huffman, const u8* code_lengths, u32 code_lengths_size,
                         Error* err) {
  u32 sizes[17] = {};
  for (u32 i = 0; i < code_lengths_size; ++i) ++sizes[code_lengths[i]];
  sizes[0] = 0;
  for (u32 i = 1; i < 16; ++i) ERROR_ASSERT(sizes[i] <= static_cast<u32>(1 << i), *err,
                                            Error::PngBadSizes,);

  u32 k = 0;
  u32 code = 0;
  u32 next_code[16] = {};
  for (u32 i = 1; i < 16; ++i) {
    next_code[i] = code;
    huffman.first_code[i] = (u16) code;
    huffman.first_symbol[i] = (u16) k;
    code += sizes[i];
    if (sizes[i]) ERROR_ASSERT(code - 1 < static_cast<u32>(1 << i), *err, Error::PngBadCodeLengths,);
    huffman.max_code[i] = code << (16 - i);
    code <<= 1;
    k += sizes[i];
  }
  huffman.max_code[16] = 0x10000;

  std::memset(huffman.fast_table, 0, sizeof(huffman.fast_table));
  for (u32 i = 0; i < code_lengths_size; ++i) {
    u8 code_length = code_lengths[i];
    if (!code_length) continue;

    u32 canonical_pos = next_code[code_length] -
      huffman.first_code[code_length] + huffman.first_symbol[code_length];
    u16 fast_table_val = (u16) ((u32) (code_length << ZLIB_FAST_BITS) | i);
    huffman.length[canonical_pos] = (u8) code_length;
    huffman.value[canonical_pos] = (u16) i;
    if (code_length <= ZLIB_FAST_BITS) {
      u32 j = bit_reverse((u16) next_code[code_length], code_length);
      while (j < (1 << ZLIB_FAST_BITS)) {
        huffman.fast_table[j] = fast_table_val;
        j += (1 << code_length);
      }
    }
    ++next_code[code_length];
  }
}

inline static bool
zlib_eof(ZlibContext& zlib_ctx) {
  return zlib_ctx.data >= zlib_ctx.data_end;
}

static u32
zlib_huffman_decode(ZlibContext& zlib_ctx, Huffman& huffman, Error* err) {
  if (zlib_ctx.bits_buffered < 16) {
    if (zlib_eof(zlib_ctx)) {
      ERROR_ASSERT(!zlib_ctx.hit_eof_once, *err, Error::PngCorruptZlib, (u32) -1);
      zlib_ctx.hit_eof_once = true;
      zlib_ctx.bits_buffered += 16;
    }
    else zlib_fill_bits(zlib_ctx);
  }

  // NOTE(szulf): trying to decode it through the fast lookup table
  u16 fast_value = huffman.fast_table[zlib_ctx.bits_buffer & ZLIB_FAST_MASK];
  if (fast_value) {
    u16 code_length = fast_value >> 9;
    zlib_ctx.bits_buffer >>= code_length;
    zlib_ctx.bits_buffered -= code_length;
    return fast_value & 0b111111111;
  }

  // NOTE(szulf): lookup table failed, doing it the slow way
  u32 bits = bit_reverse((u16) zlib_ctx.bits_buffer, 16);
  u32 code_length = ZLIB_FAST_BITS + 1;
  while (true) {
    if (bits < huffman.max_code[code_length]) break;
    ++code_length;
  }
  ERROR_ASSERT(code_length < 16, *err, Error::PngCorruptZlib, (u32) -1);

  u32 value_idx = (bits >> (16 - code_length)) -
    huffman.first_code[code_length] + huffman.first_symbol[code_length];
  ERROR_ASSERT(value_idx < ZLIB_NUM_SYMBOLS_LITERAL_LENGTH,
               *err, Error::PngCorruptZlib, (u32) -1);
  ERROR_ASSERT(huffman.length[value_idx] == code_length, *err, Error::PngCorruptZlib, (u32) -1);
  zlib_ctx.bits_buffer >>= code_length;
  zlib_ctx.bits_buffered -= code_length;
  return huffman.value[value_idx];
}

static void
zlib_huffman_compute_codes(ZlibContext& zlib_ctx, Error* err) {
  static const u8 code_length_indices[19] =
    {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
  Error error = Error::Success;

  u32 hlit = zlib_read_bits(zlib_ctx, 5) + 257;
  u32 hdist = zlib_read_bits(zlib_ctx, 5) + 1;
  u32 hclen = zlib_read_bits(zlib_ctx, 4) + 4;
  u32 total_iterations = hlit + hdist;

  u8 code_length_sizes[19] = {};
  Huffman code_length_tree = {};
  for (u32 i = 0; i < hclen; ++i) {
    u8 code_length = (u8) zlib_read_bits(zlib_ctx, 3);
    code_length_sizes[code_length_indices[i]] = code_length;
  }
  zlib_huffman_build(code_length_tree, code_length_sizes, 19, &error);
  ERROR_ASSERT(error == Error::Success, *err, error,);

  u32 iteration_idx{0};
  u8 code_lengths[286 + 32 + 137]{};
  while (iteration_idx < total_iterations) {
    u32 decoded_value = zlib_huffman_decode(zlib_ctx, code_length_tree, &error);
    ERROR_ASSERT((decoded_value < 19), *err, Error::PngBadCodeLengths,);
    ERROR_ASSERT(error == Error::Success, *err, error,);
    if (decoded_value < 16) {
      code_lengths[iteration_idx++] = (u8) decoded_value;
    } else {
      u8 fill = 0;
      u32 repeat_times;
      if (decoded_value == 16) {
        repeat_times = zlib_read_bits(zlib_ctx, 2) + 3;
        ERROR_ASSERT(iteration_idx != 0, *err, Error::PngBadCodeLengths,);
        fill = code_lengths[iteration_idx - 1];
      } else if (decoded_value == 17) {
        repeat_times = zlib_read_bits(zlib_ctx, 3) + 3;
      } else {
        ERROR_ASSERT(decoded_value == 18, *err, Error::PngBadCodeLengths,);
        repeat_times = zlib_read_bits(zlib_ctx, 7) + 11;
      }
      ERROR_ASSERT(total_iterations - iteration_idx >= repeat_times,
                   *err, Error::PngBadCodeLengths,);
      std::memset(code_lengths + iteration_idx, fill, repeat_times);
      iteration_idx += repeat_times;
    }
  }
  ERROR_ASSERT(iteration_idx == total_iterations, *err, Error::PngBadCodeLengths,);
  zlib_huffman_build(zlib_ctx.length, code_lengths, hlit, &error);
  ERROR_ASSERT(error == Error::Success, *err, error,);
  zlib_huffman_build(zlib_ctx.distance, code_lengths + hlit, hdist, &error);
  ERROR_ASSERT(error == Error::Success, *err, error,);
}

static consteval std::uint32_t
png_type(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d) {
  return static_cast<std::uint32_t>((a << 24) | (b << 16) | (c << 8) | d);
}

static const u16 length_code_to_base_length[31] = {
  3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35,
  43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0,
};
static const u8 length_code_to_extra_bits[31] = {
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0
};

static const u16 distance_code_to_base_length[32] = {
  1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
  769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0, 0,
};
static const u8 distance_code_to_extra_bits[32] = {
  0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

static void
zlib_huffman_parse_block(ZlibContext& zlib_ctx, Error* err) {
  Error error = Error::Success;

  while (true) {
    u32 decoded_value = zlib_huffman_decode(zlib_ctx, zlib_ctx.length, &error);
    ERROR_ASSERT(error == Error::Success, *err, error,);
    if (decoded_value == 256) {
      ERROR_ASSERT(!(zlib_ctx.hit_eof_once && zlib_ctx.bits_buffered < 16), *err,
                   Error::PngUnexpectedEnd,);
      return;
    } else if (decoded_value < 256) {
      zlib_ctx.out.push_back(static_cast<u8>(decoded_value));
    } else {
      ERROR_ASSERT(decoded_value < 286, *err, Error::PngBadHuffmanCode,);
      decoded_value -= 257;
      u32 len = length_code_to_base_length[decoded_value];
      if (length_code_to_extra_bits[decoded_value]) {
        len += zlib_read_bits(zlib_ctx, length_code_to_extra_bits[decoded_value]);
      }

      decoded_value = zlib_huffman_decode(zlib_ctx, zlib_ctx.distance, &error);
      ERROR_ASSERT(error == Error::Success, *err, error,);
      ERROR_ASSERT(decoded_value < 30, *err, Error::PngBadHuffmanCode,);
      u32 dist = distance_code_to_base_length[decoded_value];
      if (distance_code_to_extra_bits[decoded_value]) {
        dist += zlib_read_bits(zlib_ctx, distance_code_to_extra_bits[decoded_value]);
      }
      ERROR_ASSERT(zlib_ctx.out.size() >= dist, *err, Error::PngBadDistance,);

      usize p_idx = zlib_ctx.out.size() - dist;
      if (dist == 1) while (len--) {
        zlib_ctx.out.push_back(zlib_ctx.out[p_idx]);
      } else while (len--) {
        zlib_ctx.out.push_back(zlib_ctx.out[p_idx++]);
      }
    }
  }
}

enum class PngFilterMethod : u8 {
  None,
  Sub,
  Up,
  Average,
  Paeth,
  AverageFirstLine,
};

PngFilterMethod first_line_filter_method[5] = {
  PngFilterMethod::None,
  PngFilterMethod::Sub,
  PngFilterMethod::None,
  PngFilterMethod::AverageFirstLine,
  PngFilterMethod::Sub,
};

static i32
png_paeth_predictor(i32 a, i32 b, i32 c) {
  i32 p = a + b - c;
  i32 pa = std::abs(p - a);
  i32 pb = std::abs(p - b);
  i32 pc = std::abs(p - c);
  if (pa <= pb && pa <= pc) return a;
  else if (pb <= pc) return b;
  else return c;
}

static void
png_create_rgba8(Image& img, Context& ctx, std::vector<u8> data, Error* err) {
  u8 channels_in = color_format_to_number_of_channels[(usize) ctx.color_format];
  u8 byte_depth = ctx.bit_depth == 16 ? 2 : 1;
  u32 data_bytes_per_pixel = byte_depth * channels_in;
  usize stride = img.width * Image::channels;
  usize data_idx = 0;

  img.data.resize(img.width * img.height * img.channels);

  // NOTE(szulf): (x + 0b111) >> 3 is basically doing ceil(x / 8.0f)
  usize bytes_per_scanline = ((img.width * channels_in * ctx.bit_depth) + 0b111) >> 3;
  usize filter_buffer_size = bytes_per_scanline * 2;
  std::vector<std::uint8_t> filter_buffer{};
  filter_buffer.resize(filter_buffer_size);

  usize width = img.width;
  if (ctx.bit_depth < 8) {
    data_bytes_per_pixel = 1;
    width = bytes_per_scanline;
  }

  std::span<std::uint8_t> curr{filter_buffer.data(), bytes_per_scanline};
  std::span<std::uint8_t> prev{filter_buffer.data() + bytes_per_scanline, bytes_per_scanline};
  for (u32 line_idx = 0; line_idx < img.height; ++line_idx) {
    u32 filter_method_int = data[data_idx++];
    ERROR_ASSERT(filter_method_int < 5, *err, Error::PngInvalidFilter,);
    auto filter_method = static_cast<PngFilterMethod>(filter_method_int);
    if (line_idx == 0) filter_method = first_line_filter_method[(usize) filter_method];
    usize data_bytes_per_line = data_bytes_per_pixel * width;
    u8* dest = img.data.data() + (stride * line_idx);

    switch (filter_method) {
      case PngFilterMethod::None: {
        for (usize i = 0; i < data_bytes_per_line; ++i) {
          curr[i] = data[data_idx + i];
        }
      } break;
      case PngFilterMethod::Sub: {
        for (usize i = 0; i < data_bytes_per_pixel; ++i) {
          curr[i] = data[data_idx + i];
        }
        for (u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i) {
          curr[i] = (data[data_idx + i] + curr[i - data_bytes_per_pixel]) & 255;
        }
      } break;
      case PngFilterMethod::Up: {
        for (u32 i = 0; i < data_bytes_per_line; ++i) {
          curr[i] = (data[data_idx + i] + prev[i]) & 255;
        }
      } break;
      case PngFilterMethod::Average: {
        for (u32 i = 0; i < data_bytes_per_pixel; ++i) {
          curr[i] = (data[data_idx + i] + (prev[i] / 2)) & 255;
        }
        for (u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i) {
          curr[i] = static_cast<u8>((data[data_idx + i] +
                                    ((prev[i] + curr[i - data_bytes_per_pixel]) / 2)) & 255);
        }
      } break;
      case PngFilterMethod::Paeth: {
        for (u32 i = 0; i < data_bytes_per_pixel; ++i) {
          curr[i] = (data[data_idx + i] + prev[i]) & 255;
        }
        for (u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i) {
          curr[i] = static_cast<u8>((
              data[data_idx + i] +
              png_paeth_predictor(curr[i - data_bytes_per_pixel],
                                        prev[i], prev[i - data_bytes_per_pixel])
            ) & 255);
        }
      } break;
      case PngFilterMethod::AverageFirstLine: {
        for (usize i = 0; i < data_bytes_per_pixel; ++i) {
          curr[i] = data[data_idx + i];
        }
        for (u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i) {
          curr[i] = (data[data_idx + i] + curr[i - data_bytes_per_pixel]) & 255;
        }
      } break;
    }
    data_idx += data_bytes_per_line;

    if (ctx.bit_depth < 8) {
      TODO("support tsmaller depths");
    } else if (ctx.bit_depth == 8) {
      if (channels_in == Image::channels) {
        std::ranges::copy_n(curr.data(), static_cast<std::int64_t>(width * Image::channels), dest);
      } else if (channels_in == 1) {
      } else {
        ASSERT(channels_in == 3, "png only a/llows 1, 3, 4 channels");
        for (u32 i = 0; i < img.width; ++i) {
          dest[i * 4 + 0] = curr[i * 3 + 0];
          dest[i * 4 + 1] = curr[i * 3 + 1];
          dest[i * 4 + 2] = curr[i * 3 + 2];
          dest[i * 4 + 3] = 255;
        }
      }
    } else if (ctx.bit_depth == 16) {
      TODO("support 16bit depth");
    }

    std::swap(curr, prev);
  }
}

static Image
decode_png(const std::filesystem::path& path, Error* err) {
  Image img{};
  Error error = Error::Success;

  std::string data = platform::read_entire_file(path);

  Context ctx{};
  ctx.data = std::ranges::to<std::vector<std::uint8_t>>(data);

  // NOTE(szulf): check png header
  static const u8 png_header[] = {137, 80, 78, 71, 13, 10, 26, 10};
  for (u32 i = 0; i < 8; ++i) {
    ERROR_ASSERT(get8(ctx) == png_header[i], *err, Error::PngInvalidHeader, img);
  }

  bool first = true;
  bool running = true;
  std::vector<u8> combined_idat_chunks{};
  while (running && ctx.data_idx < ctx.data.size()) {
    PngChunk chunk = {};
    chunk.length = get32be(ctx);
    chunk.type = get32be(ctx);

    switch (chunk.type) {
      case png_type('I', 'H', 'D', 'R'): {
        ERROR_ASSERT(first, *err, Error::PngIhdrNotFirst, img);
        first = false;
        ERROR_ASSERT(chunk.length == 13, *err, Error::PngInvalidIhdr, img);
        img.width = get32be(ctx);
        img.height = get32be(ctx);
        ERROR_ASSERT(img.width < MAX_SIZE && img.width > 0 &&
                     img.height < MAX_SIZE && img.height > 0,
                     *err, Error::PngInvalidIhdr, img);

        u8 bit_depth = get8(ctx);
        ERROR_ASSERT(bit_depth > 0 && bit_depth <= 16, *err, Error::PngInvalidIhdr, img);
        ctx.bit_depth = bit_depth;
        u8 color = get8(ctx);
        ERROR_ASSERT(color == 0 || color == 2 || color == 3 || color == 4 || color == 6,
                     *err, Error::PngInvalidIhdr, img);
        ctx.color_format = color_to_color_format[color];

        u8 compression = get8(ctx);
        ERROR_ASSERT(compression == 0, *err, Error::PngInvalidIhdr, img);
        u8 filter = get8(ctx);
        ERROR_ASSERT(filter == 0, *err, Error::PngInvalidIhdr, img);
        ctx.interlaced = get8(ctx);
      } break;

      case png_type('P', 'L', 'T', 'E'): {
        TODO("support palettes");
      } break;

      case png_type('I', 'D', 'A', 'T'): {
        ERROR_ASSERT(!first, *err, Error::PngIhdrNotFirst, img);
        ERROR_ASSERT(chunk.length <= MAX_IDAT_CHUNK_SIZE, *err,
                     Error::PngInvalidIdat, img);
        combined_idat_chunks.reserve(combined_idat_chunks.size() + chunk.length);
        for (usize i = 0; i < chunk.length; ++i) {
          combined_idat_chunks.push_back(ctx.data[ctx.data_idx + i]);
        }
        ctx.data_idx += chunk.length;
      } break;

      case png_type('I', 'E', 'N', 'D'): {
        ERROR_ASSERT(!first, *err, Error::PngIhdrNotFirst, img);
        ERROR_ASSERT(combined_idat_chunks.size() != 0, *err, Error::PngInvalidIdat, img);
        ERROR_ASSERT(error == Error::Success, *err, error, img);

        // NOTE(szulf): zlib parsing
        ZlibContext zlib_ctx = {};
        zlib_ctx.data = combined_idat_chunks.data();
        zlib_ctx.data_end = combined_idat_chunks.data() + combined_idat_chunks.size();

        { // NOTE(szulf): header
          u8 cmf = zlib_get8(zlib_ctx);
          u8 cm = cmf & 15;
          u8 flg = zlib_get8(zlib_ctx);
          ERROR_ASSERT(zlib_data_in_bounds(zlib_ctx), *err, Error::PngCorruptZlib, img);
          ERROR_ASSERT((u16) (cmf * 256 + flg) % 31 == 0, *err, Error::PngCorruptZlib, img);
          ERROR_ASSERT(!(flg & 32), *err, Error::PngCorruptZlib, img);
          ERROR_ASSERT(cm == 8, *err, Error::PngCorruptZlib, img);
        }

        // NOTE(szulf): data
        zlib_ctx.bits_buffered = 0;
        zlib_ctx.bits_buffer = 0;
        bool final = false;
        PngCompressionType type;
        do {
          final = zlib_read_bits(zlib_ctx, 1);
          type = (PngCompressionType) zlib_read_bits(zlib_ctx, 2);
          switch (type) {
            case PngCompressionType::Uncompressed: {
              if (zlib_ctx.bits_buffered & 0b111) {
                zlib_read_bits(zlib_ctx, zlib_ctx.bits_buffered & 0b111);
              }
              u8 header[4];
              u32 k = 0;
              while (zlib_ctx.bits_buffered > 0) {
                header[k++] = (u8) (zlib_ctx.bits_buffer & 0b11111111);
                zlib_ctx.bits_buffer >>= 8;
                zlib_ctx.bits_buffered -= 8;
              }

              while (k < 4) {
                header[k++] = zlib_get8(zlib_ctx);
              }
              u16 len = (u16) (header[1] << 8) | header[0];
              u16 nlen = (u16) (header[3] << 8) | header[2];
              ERROR_ASSERT(nlen == (len ^ 0xFFFF), *err, Error::PngCorruptZlib, img);
              ERROR_ASSERT(zlib_ctx.data + len <= zlib_ctx.data_end, *err,
                           Error::PngReadPastBuffer, img);
              zlib_ctx.out.reserve(zlib_ctx.out.size() + len);
              for (usize i = 0; i < len; ++i) {
                zlib_ctx.out.push_back(zlib_ctx.data[i]);
              }
              zlib_ctx.data += len;
            } break;

            case PngCompressionType::FixedHuffman: {
              zlib_huffman_build(zlib_ctx.length, image_zlib_fixed_huffman_length_alphabet,
                                       ZLIB_NUM_SYMBOLS_LITERAL_LENGTH, &error);
              ERROR_ASSERT(error == Error::Success, *err, error, img);
              zlib_huffman_build(zlib_ctx.distance,
                                       image_zlib_fixed_huffman_distance_alphabet, 32, &error);
              ERROR_ASSERT(error == Error::Success, *err, error, img);
              zlib_huffman_parse_block(zlib_ctx, &error);
              ERROR_ASSERT(error == Error::Success, *err, error, img);
            } break;

            case PngCompressionType::DynamicHuffman: {
              zlib_huffman_compute_codes(zlib_ctx, &error);
              ERROR_ASSERT(error == Error::Success, *err, error, img);
              zlib_huffman_parse_block(zlib_ctx, &error);
              ERROR_ASSERT(error == Error::Success, *err, error, img);
            } break;

            case PngCompressionType::Illegal:
            default: {
              ERROR_ASSERT(false, *err, Error::PngIllegalCompresionType, img);
            } break;
          }
        } while (!final);

        if (!ctx.interlaced) {
          png_create_rgba8(img, ctx, zlib_ctx.out, &error);
          ERROR_ASSERT(error == Error::Success, *err, error, img);
        } else {
          TODO("support interlaced pngs");
        }

        get32be(ctx);
        running = false;
      } break;

      default: {
        ERROR_ASSERT(!first, *err, Error::PngIhdrNotFirst, img);
        ctx.data_idx += chunk.length;
      } break;
    }
    // NOTE(szulf): skip CRC
    get32be(ctx);
  }

  *err = Error::Success;
  return img;
}

}
