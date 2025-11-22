#include "engine/image.hpp"

#include "badtl/array.hpp"
#include "badtl/utils.hpp"
#include "badtl/files.hpp"
#include "badtl/list.hpp"
#include "badtl/memory.hpp"
#include "badtl/types.hpp"
#include "badtl/math.hpp"

namespace core {

namespace image_impl {

struct Chunk final {
  btl::u8* data;
  btl::u32 length;
  btl::u32 type;
};

enum class ColorFormat {
  Grayscale = 0,
  Rgb = 2,
  Palette = 3,
  GrayscaleAlpha = 4,
  Rgba = 6,
};

static constexpr btl::Array<btl::u8, 7> color_format_to_number_of_channels =
  {1, static_cast<btl::u8>(-1), 3, 1, 2, static_cast<btl::u8>(-1), 4};

static constexpr btl::u8 RGBA_CHANNELS = 4;
static constexpr btl::u8 RGBA8_BYTES_PER_PIXEL = 4;

enum class FilterMethod {
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

static btl::i32 paeth_predictor(btl::i32 a, btl::i32 b, btl::i32 c) {
  btl::i32 p = a + b - c;
  btl::i32 pa = btl::abs(p - a);
  btl::i32 pb = btl::abs(p - b);
  btl::i32 pc = btl::abs(p - c);
  if (pa <= pb && pa <= pc) {
    return a;
  } else if (pb <= pc) {
    return b;
  } else {
    return c;
  }
}

struct Context {
  btl::u8* data;
  btl::u8* data_end;
  btl::u8 bit_depth;
  bool interlaced;
  ColorFormat color_format;

  btl::u8 get8() {
    if (data < data_end) {
      return *data++;
    }
    return static_cast<btl::u8>(-1);
  }

  btl::u16 get16be() {
    btl::u16 v = get8();
    return static_cast<btl::u16>((v << 8) + get8());
  }

  btl::u32 get32be() {
    btl::u32 v = get16be();
    return (v << 16) + get16be();
  }

  btl::Result<void, ImageError>
  create_rgba_8(Image& img, btl::u8* zlib_data, btl::Allocator& allocator, btl::Allocator& scratch_arena) {
    btl::u8 channels = image_impl::color_format_to_number_of_channels[static_cast<btl::usize>(color_format)];
    btl::u8 byte_depth = bit_depth == 16 ? 2 : 1;
    btl::u32 data_bytes_per_pixel = byte_depth * channels;
    btl::usize stride = img.width * RGBA8_BYTES_PER_PIXEL;

    img.data = static_cast<btl::u8*>(allocator.alloc(img.width * img.height * RGBA8_BYTES_PER_PIXEL));

    // NOTE(szulf): (x + 0b111) >> 3 is basically doing ceil(x / 8.0f)
    btl::usize bytes_per_scanline = ((img.width * channels * bit_depth) + 7) >> 3;
    auto* filter_buffer = static_cast<btl::u8*>(scratch_arena.alloc(bytes_per_scanline * 2));

    btl::usize width = img.width;
    if (bit_depth < 8) {
      data_bytes_per_pixel = 1;
      width = bytes_per_scanline;
    }

    btl::u8* curr = filter_buffer;
    btl::u8* prev = filter_buffer + bytes_per_scanline;
    for (btl::u32 line_idx = 0; line_idx < img.height; ++line_idx) {
      btl::u8 filter_method_val = *zlib_data++;
      if (filter_method_val > 4) {
        return btl::err<void>(ImageError::InvalidFilter);
      }
      if (line_idx == 0) {
        filter_method_val = static_cast<btl::u8>(first_line_filter_method[filter_method_val]);
      }
      btl::usize data_bytes_per_line = data_bytes_per_pixel * width;
      btl::u8* dest = img.data + (stride * line_idx);

      image_impl::FilterMethod filter_method = static_cast<image_impl::FilterMethod>(filter_method_val);
      switch (filter_method) {
        case image_impl::FilterMethod::None: {
          btl::mem::copy(curr, zlib_data, data_bytes_per_line);
        } break;
        case image_impl::FilterMethod::Sub: {
          btl::mem::copy(curr, zlib_data, data_bytes_per_pixel);
          for (btl::u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i) {
            curr[i] = (zlib_data[i] + curr[i - data_bytes_per_pixel]) & 255;
          }
        } break;
        case image_impl::FilterMethod::Up: {
          for (btl::u32 i = 0; i < data_bytes_per_line; ++i) {
            curr[i] = (zlib_data[i] + prev[i]) & 255;
          }
        } break;
        case image_impl::FilterMethod::Average: {
          for (btl::u32 i = 0; i < data_bytes_per_pixel; ++i) {
            curr[i] = (zlib_data[i] + (prev[i] / 2)) & 255;
          }
          for (btl::u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i) {
            curr[i] = ((zlib_data[i] + ((prev[i] + curr[i - data_bytes_per_pixel]) / 2)) & 255);
          }
        } break;
        case image_impl::FilterMethod::Paeth: {
          for (btl::u32 i = 0; i < data_bytes_per_pixel; ++i) {
            curr[i] = (zlib_data[i] + prev[i]) & 255;
          }
          for (btl::u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i) {
            curr[i] = (zlib_data[i] +
                       paeth_predictor(curr[i - data_bytes_per_pixel], prev[i], prev[i - data_bytes_per_pixel])) &
                      255;
          }
        } break;
        case image_impl::FilterMethod::AverageFirstLine: {
          btl::mem::copy(curr, zlib_data, data_bytes_per_pixel);
          for (btl::u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i) {
            curr[i] = (zlib_data[i] + curr[i - data_bytes_per_pixel]) & 255;
          }
        } break;
      }
      zlib_data += data_bytes_per_line;

      if (bit_depth == 8) {
        if (channels == RGBA_CHANNELS) {
          btl::mem::copy(dest, curr, width * RGBA_CHANNELS);
        } else if (channels == 1) {
          ASSERT(false, "[TODO] support grayscale");
        } else {
          ASSERT(channels == 3, "png only allows 1, 3, 4 channels");
          for (btl::u32 i = 0; i < img.width; ++i) {
            dest[i * 4 + 0] = curr[i * 3 + 0];
            dest[i * 4 + 1] = curr[i * 3 + 1];
            dest[i * 4 + 2] = curr[i * 3 + 2];
            dest[i * 4 + 3] = 255;
          }
        }
      } else if (bit_depth < 8) {
        ASSERT(false, "[TODO] support smaller depths");
      } else if (bit_depth == 16) {
        ASSERT(false, "[TODO] support 16 bit depth");
      }

      auto* temp = curr;
      curr = prev;
      prev = temp;
    }
    return btl::ok<ImageError>();
  }
};

static constexpr btl::u32 IHDR = ('I' << 24) | ('H' << 16) | ('D' << 8) | ('R');
static constexpr btl::u32 PLTE = ('P' << 24) | ('L' << 16) | ('T' << 8) | ('E');
static constexpr btl::u32 IDAT = ('I' << 24) | ('D' << 16) | ('A' << 8) | ('T');
static constexpr btl::u32 IEND = ('I' << 24) | ('E' << 16) | ('N' << 8) | ('D');
static constexpr btl::u32 MAX_IDAT_CHUNK_SIZE = 1u << 30;

inline static auto bit_reverse_16(btl::u16 n) -> btl::u16 {
  n = ((n & 0xAAAA) >> 1) | static_cast<btl::u16>((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >> 2) | static_cast<btl::u16>((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >> 4) | static_cast<btl::u16>((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >> 8) | static_cast<btl::u16>((n & 0x00FF) << 8);
  return n;
}

inline static auto bit_reverse(btl::u16 v, btl::u8 bits) -> btl::u16 {
  ASSERT(bits <= 16, "this function doesnt reverse more than 16 bits");
  return bit_reverse_16(v) >> (16 - bits);
}

namespace zlib {

static constexpr btl::u8 FAST_BITS = 9;
static constexpr btl::u16 FAST_MASK = (1 << FAST_BITS) - 1;
static constexpr btl::u16 NUM_SYMBOLS_LITERAL_LENGTH = 288;

struct Context;
struct Huffman {
  btl::u16 value[NUM_SYMBOLS_LITERAL_LENGTH];
  btl::u8 length[NUM_SYMBOLS_LITERAL_LENGTH];
  btl::u32 max_code[17];
  btl::u16 first_code[16];
  btl::u16 first_symbol[16];
  btl::u16 fast_table[1 << FAST_BITS];

  btl::Result<void, ImageError> build(const btl::u8* code_lengths, btl::usize code_lengths_size);
  btl::Result<btl::u32, ImageError> decode(Context& zlib_ctx);
};

static constexpr btl::u8 fixed_huffman_length_alphabet[NUM_SYMBOLS_LITERAL_LENGTH] = {
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8,
};

static constexpr btl::u8 fixed_huffman_distance_alphabet[32] = {
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
};

static constexpr btl::u16 length_code_to_base_length[31] = {
  3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27, 31,
  35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0,
};
static constexpr btl::Array<btl::u8, 31> length_code_to_extra_bits = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                                                                      3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0};

static constexpr btl::u16 distance_code_to_base_length[32] = {
  1,   2,   3,   4,   5,    7,    9,    13,   17,   25,   33,   49,    65,    97,    129, 193,
  257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0,   0,
};
static constexpr btl::Array<btl::u8, 32> distance_code_to_extra_bits = {0, 0, 0,  0,  1,  1,  2,  2,  3,  3,
                                                                        4, 4, 5,  5,  6,  6,  7,  7,  8,  8,
                                                                        9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

struct Context {
  Huffman length;
  Huffman distance;
  btl::List<btl::u8> out;
  btl::u8* data;
  btl::u8* data_end;
  btl::u32 bits_buffered;
  btl::u32 bits_buffer;
  bool hit_eof_once;

  bool data_in_bounds() {
    return data < data_end;
  }

  btl::u8 get8() {
    if (data_in_bounds()) {
      return *data++;
    }
    return static_cast<btl::u8>(-1);
  }

  void fill_bits() {
    do {
      if (bits_buffer >= (1u << bits_buffered)) {
        data = data_end;
        return;
      }
      bits_buffer |= static_cast<btl::u32>(get8() << bits_buffered);
      bits_buffered += 8;
    } while (bits_buffered <= 24);
  }

  btl::u32 read_bits(btl::u32 n) {
    btl::u32 k = 0;
    if (bits_buffered < n) {
      fill_bits();
    }
    k = bits_buffer & static_cast<btl::u32>((1 << n) - 1);
    bits_buffer >>= n;
    bits_buffered -= n;
    return k;
  }

  bool eof() {
    return data >= data_end;
  }

  btl::Result<void, ImageError> compute_codes() {
    static constexpr btl::Array<btl::u8, 19> code_length_indices =
      {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    btl::u32 hlit = read_bits(5) + 257;
    btl::u32 hdist = read_bits(5) + 1;
    btl::u32 hclen = read_bits(4) + 4;
    btl::u32 total_iterations = hlit + hdist;

    btl::u8 code_length_sizes[19] = {};
    Huffman code_length_tree = {};
    for (btl::u32 i = 0; i < hclen; ++i) {
      btl::u8 code_length = static_cast<btl::u8>(read_bits(3));
      code_length_sizes[code_length_indices[i]] = code_length;
    }
    auto code_length_tree_res = code_length_tree.build(code_length_sizes, 19);
    if (code_length_tree_res.has_err) {
      return {code_length_tree_res};
    }

    btl::u32 iteration_idx = {};
    btl::u8 code_lengths[286 + 32 + 137] = {};
    while (iteration_idx < total_iterations) {
      auto decoded_value_res = code_length_tree.decode(*this);
      if (decoded_value_res.has_err) {
        return btl::err<void>(decoded_value_res.value.error);
      }
      auto& decoded_value = decoded_value_res.value.success;
      if (decoded_value > 18) {
        return btl::err<void>(ImageError::BadCodeLength);
      }
      if (decoded_value < 16) {
        code_lengths[iteration_idx++] = static_cast<btl::u8>(decoded_value);
      } else {
        btl::u8 fill = 0;
        btl::u32 repeat_times;
        if (decoded_value == 16) {
          repeat_times = read_bits(2) + 3;
          if (iteration_idx == 0) {
            return btl::err<void>(ImageError::BadCodeLength);
          }
          fill = code_lengths[iteration_idx - 1];
        } else if (decoded_value == 17) {
          repeat_times = read_bits(3) + 3;
        } else {
          repeat_times = read_bits(7) + 11;
        }
        if (total_iterations - iteration_idx < repeat_times) {
          return btl::err<void>(ImageError::BadCodeLength);
        }
        btl::mem::set(code_lengths + iteration_idx, fill, repeat_times);
        iteration_idx += repeat_times;
      }
    }
    if (iteration_idx != total_iterations) {
      return btl::err<void>(ImageError::BadCodeLength);
    }
    auto length_build_res = length.build(code_lengths, hlit);
    if (length_build_res.has_err) {
      return {length_build_res};
    }
    auto distance_build_res = distance.build(code_lengths + hlit, hdist);
    if (distance_build_res.has_err) {
      return {distance_build_res};
    }

    return btl::ok<ImageError>();
  }

  btl::Result<void, ImageError> parse_block() {
    while (true) {
      auto decoded_value_res = length.decode(*this);
      if (decoded_value_res.has_err) {
        return btl::err<void>(decoded_value_res.value.error);
      }
      auto& decoded_value = decoded_value_res.value.success;
      if (decoded_value == 256) {
        if (hit_eof_once && bits_buffered >= 16) {
          return btl::err<void>(ImageError::UnexpectedEnd);
        }
        return btl::ok<ImageError>();
      } else if (decoded_value < 256) {
        out.push_dynamic(static_cast<btl::u8>(decoded_value));
      } else {
        if (decoded_value > 285) {
          return btl::err<void>(ImageError::BadHuffmanCode);
        }
        decoded_value -= 257;
        btl::u32 len = length_code_to_base_length[decoded_value];
        if (length_code_to_extra_bits[decoded_value]) {
          len += read_bits(length_code_to_extra_bits[decoded_value]);
        }

        decoded_value_res = distance.decode(*this);
        if (decoded_value_res.has_err) {
          return btl::err<void>(decoded_value_res.value.error);
        }
        if (decoded_value > 29) {
          return btl::err<void>(ImageError::BadHuffmanCode);
        }
        btl::u32 dist = distance_code_to_base_length[decoded_value];
        if (distance_code_to_extra_bits[decoded_value]) {
          dist += read_bits(distance_code_to_extra_bits[decoded_value]);
        }
        if (out.size < dist) {
          return btl::err<void>(ImageError::BadDistance);
        }

        btl::usize p_idx = out.size - dist;
        while (len--) {
          out.push_dynamic(out[p_idx++]);
        }
      }
    }
  }
};

btl::Result<void, ImageError> Huffman::build(const btl::u8* code_lengths, btl::usize code_lengths_size) {
  btl::u32 sizes[17] = {};
  for (btl::u32 i = 0; i < code_lengths_size; ++i) {
    ++sizes[code_lengths[i]];
  }
  sizes[0] = 0;
  for (btl::u32 i = 1; i < 16; ++i) {
    if (sizes[i] > static_cast<btl::u32>(1 << i)) {
      return btl::err<void>(ImageError::BadSizes);
    }
  }

  btl::u32 k = 0;
  btl::u32 code = 0;
  btl::u32 next_code[16] = {};
  for (btl::u32 i = 1; i < 16; ++i) {
    next_code[i] = code;
    first_code[i] = static_cast<btl::u16>(code);
    first_symbol[i] = static_cast<btl::u16>(k);
    code += sizes[i];
    if (sizes[i]) {
      if (code - 1 >= static_cast<btl::u32>(1 << i)) {
        return btl::err<void>(ImageError::BadCodeLength);
      }
    }
    max_code[i] = code << (16 - i);
    code <<= 1;
    k += sizes[i];
  }
  max_code[16] = 0x10000;

  btl::mem::set(fast_table, 0, sizeof(fast_table));
  for (btl::u32 i = 0; i < code_lengths_size; ++i) {
    btl::u8 code_length = code_lengths[i];
    if (!code_length) {
      continue;
    }

    btl::u32 canonical_pos = next_code[code_length] - first_code[code_length] + first_symbol[code_length];
    btl::u16 fast_table_val = static_cast<btl::u16>(static_cast<btl::u32>(code_length << FAST_BITS) | i);
    length[canonical_pos] = static_cast<btl::u8>(code_length);
    value[canonical_pos] = static_cast<btl::u16>(i);
    if (code_length <= FAST_BITS) {
      btl::u32 j = bit_reverse(static_cast<btl::u16>(next_code[code_length]), code_length);
      while (j < (1 << FAST_BITS)) {
        fast_table[j] = fast_table_val;
        j += (1 << code_length);
      }
    }
    ++next_code[code_length];
  }
  return btl::ok<ImageError>();
}

btl::Result<btl::u32, ImageError> Huffman::decode(Context& zlib_ctx) {
  if (zlib_ctx.bits_buffered < 16) {
    if (zlib_ctx.eof()) {
      if (zlib_ctx.hit_eof_once) {
        return btl::err<btl::u32>(ImageError::CorruptZlib);
      }
      zlib_ctx.hit_eof_once = true;
      zlib_ctx.bits_buffered += 16;
    } else {
      zlib_ctx.fill_bits();
    }
  }

  // NOTE(szulf): trying to decode it through the fast lookup table
  btl::u16 fast_value = fast_table[zlib_ctx.bits_buffer & FAST_MASK];
  if (fast_value) {
    btl::u16 code_length = fast_value >> 9;
    zlib_ctx.bits_buffer >>= code_length;
    zlib_ctx.bits_buffered -= code_length;
    // return {
    //   .has_err = false,
    //   .value = {
    //             .success = static_cast<btl::u32>(fast_value & 511),
    //             },
    // };
    return btl::ok<ImageError>(static_cast<btl::u32>(fast_value & 511));
  }

  // NOTE(szulf): lookup table failed, doing it the slow way
  btl::u32 bits = bit_reverse(static_cast<btl::u16>(zlib_ctx.bits_buffer), 16);
  btl::u32 code_length = FAST_BITS + 1;
  while (true) {
    if (bits < max_code[code_length]) {
      break;
    }
    ++code_length;
  }
  if (code_length >= 16) {
    return btl::err<btl::u32>(ImageError::CorruptZlib);
  }

  btl::u32 value_idx = (bits >> (16 - code_length)) - first_code[code_length] + first_symbol[code_length];
  if (value_idx >= NUM_SYMBOLS_LITERAL_LENGTH || length[value_idx] != code_length) {
    return btl::err<btl::u32>(ImageError::CorruptZlib);
  }
  zlib_ctx.bits_buffer >>= code_length;
  zlib_ctx.bits_buffered -= code_length;
  // return {
  //   .has_err = false,
  //   .value = {
  //             .success = static_cast<btl::u32>(value[value_idx]),
  //             },
  // };
  return btl::ok<ImageError>(static_cast<btl::u32>(value[value_idx]));
}

}

enum class CompressionType : btl::u8 {
  Uncompressed = 0,
  FixedHuffman = 1,
  DynamicHuffman = 2,
  Illegal = 3,
};

}

btl::Result<Image, ImageError> Image::from_file(const char* path, btl::Allocator& allocator) {
  return from_file(btl::String::make(path), allocator);
}

btl::Result<Image, ImageError> Image::from_file(const btl::String& path, btl::Allocator& allocator) {
  Image img = {};
  auto scratch_arena = btl::ScratchArena::get();
  defer(scratch_arena.release());
  auto file = btl::read_file(path, scratch_arena.allocator);
  image_impl::Context ctx = {};
  ctx.data = static_cast<btl::u8*>(file.ptr);
  ctx.data_end = static_cast<btl::u8*>(file.ptr) + file.size;

  static constexpr btl::Array<btl::u8, 8> png_header = {137, 80, 78, 71, 13, 10, 26, 10};
  for (const auto header : png_header) {
    if (ctx.get8() != header) {
      return btl::err<Image>(ImageError::InvalidHeader);
    }
  }

  bool first = true;
  bool running = true;
  auto combined_idat_chunks = btl::List<btl::u8>::from_dynamic_arena(scratch_arena.allocator);
  while (running && ctx.data != ctx.data_end) {
    image_impl::Chunk chunk = {};
    chunk.length = ctx.get32be();
    chunk.type = ctx.get32be();

    switch (chunk.type) {
      case image_impl::IHDR: {
        if (!first) {
          return btl::err<Image>(ImageError::IHDRNotFirst);
        }
        first = false;
        if (chunk.length != 13) {
          return btl::err<Image>(ImageError::InvalidIHDR);
        }
        img.width = ctx.get32be();
        img.height = ctx.get32be();
        if (!(img.width < MAX_SIZE && img.width > 0 && img.height < MAX_SIZE && img.height > 0)) {
          return btl::err<Image>(ImageError::InvalidIHDR);
        }

        btl::u8 bit_depth = ctx.get8();
        if (bit_depth > 16) {
          return btl::err<Image>(ImageError::InvalidIHDR);
        }
        ctx.bit_depth = bit_depth;
        btl::u8 color = ctx.get8();
        if (color != 0 && color != 2 && color != 3 && color != 4 && color != 6) {
          return btl::err<Image>(ImageError::InvalidIHDR);
        }
        ctx.color_format = static_cast<image_impl::ColorFormat>(color);
        btl::u8 compression = ctx.get8();
        if (compression != 0) {
          return btl::err<Image>(ImageError::InvalidIHDR);
        }
        btl::u8 filter = ctx.get8();
        if (filter != 0) {
          return btl::err<Image>(ImageError::InvalidIHDR);
        }
        ctx.interlaced = ctx.get8();
      } break;

      case image_impl::PLTE: {
        ASSERT(false, "[TODO] support palettes");
      } break;

      case image_impl::IDAT: {
        if (first) {
          return btl::err<Image>(ImageError::IHDRNotFirst);
        }
        if (chunk.length > image_impl::MAX_IDAT_CHUNK_SIZE) {
          return btl::err<Image>(ImageError::InvalidIDAT);
        }
        combined_idat_chunks.push_dynamic_range(ctx.data, ctx.data + chunk.length);
        ctx.data += chunk.length;
      } break;

      case image_impl::IEND: {
        if (first) {
          return btl::err<Image>(ImageError::IHDRNotFirst);
        }
        if (combined_idat_chunks.size == 0) {
          return btl::err<Image>(ImageError::InvalidIDAT);
        }
        combined_idat_chunks.dynamic_finish();

        // NOTE(szulf): zlib parsing
        image_impl::zlib::Context zlib_ctx = {};
        zlib_ctx.data = combined_idat_chunks.data;
        zlib_ctx.data_end = combined_idat_chunks.data + combined_idat_chunks.size;
        zlib_ctx.out = btl::List<btl::u8>::from_dynamic_arena(scratch_arena.allocator);
        { // NOTE(szulf): header
          btl::u8 cmf = zlib_ctx.get8();
          btl::u8 cm = cmf & 15;
          btl::u8 flg = zlib_ctx.get8();
          if (!zlib_ctx.data_in_bounds() || static_cast<btl::u16>(cmf * 256 + flg) % 31 != 0 || flg & 32 || cm != 8) {
            return btl::err<Image>(ImageError::CorruptZlib);
          }
        }

        // NOTE(szulf): data
        zlib_ctx.bits_buffered = 0;
        zlib_ctx.bits_buffer = 0;
        bool final = false;
        image_impl::CompressionType type;
        do {
          final = zlib_ctx.read_bits(1);
          type = static_cast<image_impl::CompressionType>(zlib_ctx.read_bits(2));
          switch (type) {
            case image_impl::CompressionType::Uncompressed: {
              if (zlib_ctx.bits_buffered & 7) {
                zlib_ctx.read_bits(zlib_ctx.bits_buffered & 7);
              }
              btl::u8 header[4];
              btl::u32 k = 0;
              while (zlib_ctx.bits_buffered > 0) {
                header[k++] = static_cast<btl::u8>(zlib_ctx.bits_buffer & 255);
                zlib_ctx.bits_buffer >>= 8;
                zlib_ctx.bits_buffered -= 8;
              }

              while (k < 4) {
                header[k++] = zlib_ctx.get8();
              }
              btl::u16 len = static_cast<btl::u16>(header[1] << 8) | header[0];
              btl::u16 nlen = static_cast<btl::u16>(header[3] << 8) | header[2];
              if (nlen != (len ^ 0xFFFF)) {
                return btl::err<Image>(ImageError::CorruptZlib);
              }
              if (zlib_ctx.data + len > zlib_ctx.data_end) {
                return btl::err<Image>(ImageError::ReadPastBuffer);
              }
              btl::mem::copy(zlib_ctx.out.data + zlib_ctx.out.size, zlib_ctx.data, len);
              zlib_ctx.data += len;
            } break;

            case image_impl::CompressionType::FixedHuffman: {
              auto length_build_res = zlib_ctx.length.build(
                static_cast<const btl::u8*>(image_impl::zlib::fixed_huffman_length_alphabet),
                image_impl::zlib::NUM_SYMBOLS_LITERAL_LENGTH
              );
              if (length_build_res.has_err) {
                return btl::err<Image>(length_build_res.value.error);
              }
              auto distance_build_res = zlib_ctx.distance.build(image_impl::zlib::fixed_huffman_distance_alphabet, 32);
              if (distance_build_res.has_err) {
                return btl::err<Image>(distance_build_res.value.error);
              }
              auto parse_block_res = zlib_ctx.parse_block();
              if (parse_block_res.has_err) {
                return btl::err<Image>(parse_block_res.value.error);
              }
            } break;

            case image_impl::CompressionType::DynamicHuffman: {
              auto compute_res = zlib_ctx.compute_codes();
              if (compute_res.has_err) {
                return btl::err<Image>(compute_res.value.error);
              }
              auto parse_block_res = zlib_ctx.parse_block();
              if (parse_block_res.has_err) {
                return btl::err<Image>(parse_block_res.value.error);
              }
            } break;

            case image_impl::CompressionType::Illegal: {
              return btl::err<Image>(ImageError::IllegalCompressionType);
            } break;
          }
        } while (!final);
        zlib_ctx.out.dynamic_finish();

        if (!ctx.interlaced) {
          auto res = ctx.create_rgba_8(img, zlib_ctx.out.data, allocator, scratch_arena.allocator);
          if (res.has_err) {
            return btl::err<Image>(res.value.error);
          }
        } else {
          ASSERT(false, "[TODO] support interlaced pngs");
        }

        ctx.get32be();
        running = false;
      } break;

      default: {
        if (first) {
          return btl::err<Image>(ImageError::IHDRNotFirst);
        }
        ctx.data += chunk.length;
      } break;
    }
    // NOTE(szulf): skip CRC
    ctx.get32be();
  }
  return btl::ok<ImageError>(img);
}

static btl::u8 error_image_data[] = {
  // clang-format off
  0xf3, 0x00, 0xf3, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xf3, 0x00, 0xf3,
  // clang-format on
};

Image Image::error_image() {
  return {error_image_data, 2, 2};
}

}
