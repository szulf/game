#include "image.h"

enum ImagePngColorFormat
{
  IMAGE_COLOR_FORMAT_GRAYSCALE = 0,
  IMAGE_COLOR_FORMAT_RGB = 2,
  IMAGE_COLOR_FORMAT_PALETTE = 3,
  IMAGE_COLOR_FORMAT_GRAYSCALE_ALPHA = 4,
  IMAGE_COLOR_FORMAT_RGBA = 6,
};

static const u8 color_format_to_number_of_channels[7] = {1, (u8) -1, 3, 1, 2, (u8) -1, 4};

struct ImageContext
{
  u8* data;
  u8* data_end;
  bool interlaced;
  ImagePngColorFormat color_format;
  u8 bit_depth;
};

inline static u8 image_get8(ImageContext& ctx)
{
  if (ctx.data < ctx.data_end)
  {
    return *ctx.data++;
  }
  return (u8) -1;
}

inline static u16 image_get16be(ImageContext& ctx)
{
  u16 v = image_get8(ctx);
  return (u16) (v << 8) + image_get8(ctx);
}
inline static u32 image_get32be(ImageContext& ctx)
{
  u32 v = image_get16be(ctx);
  return (v << 16) + image_get16be(ctx);
}

struct ImagePngChunk
{
  u8* data;
  u32 length;
  u32 type;
};

#define IMAGE_ZLIB_FAST_BITS 9
#define IMAGE_ZLIB_FAST_MASK ((1 << IMAGE_ZLIB_FAST_BITS) - 1)
#define IMAGE_ZLIB_NUM_SYMBOLS_LITERAL_LENGTH 288

struct ImageHuffman
{
  u16 value[IMAGE_ZLIB_NUM_SYMBOLS_LITERAL_LENGTH];
  u8 length[IMAGE_ZLIB_NUM_SYMBOLS_LITERAL_LENGTH];
  u32 max_code[17];
  u16 first_code[16];
  u16 first_symbol[16];
  u16 fast_table[1 << IMAGE_ZLIB_FAST_BITS];
};

static const u8 image_zlib_fixed_huffman_length_alphabet[IMAGE_ZLIB_NUM_SYMBOLS_LITERAL_LENGTH] = {
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

enum ImagePngCompressionType
{
  IMAGE_PNG_COMPRESSION_TYPE_UNCOMPRESSED = 0,
  IMAGE_PNG_COMPRESSION_TYPE_FIXED_HUFFMAN = 1,
  IMAGE_PNG_COMPRESSION_TYPE_DYNAMIC_HUFFMAN = 2,
  IMAGE_PNG_COMPRESSION_TYPE_ILLEGAL = 3,
};

struct ImageZlibContext
{
  bool hit_eof_once;
  u32 bits_buffered;
  u32 bits_buffer;
  u8* data;
  u8* data_end;
  u8* out;
  u8* out_start;
  ImageHuffman length;
  ImageHuffman distance;
};

inline static bool image_zlib_data_in_bounds(ImageZlibContext& zlib_ctx)
{
  return zlib_ctx.data < zlib_ctx.data_end;
}

inline static u8 image_zlib_get8(ImageZlibContext& zlib_ctx)
{
  if (image_zlib_data_in_bounds(zlib_ctx))
  {
    return *zlib_ctx.data++;
  }
  return (u8) -1;
}

static void image_zlib_fill_bits(ImageZlibContext& zlib_ctx)
{
  do
  {
    if (zlib_ctx.bits_buffer >= (1u << zlib_ctx.bits_buffered))
    {
      zlib_ctx.data = zlib_ctx.data_end;
      return;
    }
    zlib_ctx.bits_buffer |= (u32) (image_zlib_get8(zlib_ctx) << zlib_ctx.bits_buffered);
    zlib_ctx.bits_buffered += 8;
  }
  while (zlib_ctx.bits_buffered <= 24);
}

static u32 image_zlib_read_bits(ImageZlibContext& zlib_ctx, u32 n)
{
  u32 k = 0;
  if (zlib_ctx.bits_buffered < n)
  {
    image_zlib_fill_bits(zlib_ctx);
  }
  k = zlib_ctx.bits_buffer & (u32) ((1 << n) - 1);
  zlib_ctx.bits_buffer >>= n;
  zlib_ctx.bits_buffered -= n;
  return k;
}

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

static void image_zlib_huffman_build(
  ImageHuffman& huffman,
  const u8* code_lengths,
  u32 code_lengths_size,
  Error& err
)
{
  u32 sizes[17] = {0};
  for (u32 i = 0; i < code_lengths_size; ++i)
  {
    ++sizes[code_lengths[i]];
  }
  sizes[0] = 0;
  for (u32 i = 1; i < 16; ++i)
  {
    ERROR_ASSERT(sizes[i] <= (u32) (1 << i), err, IMAGE_ERROR_BAD_SIZES, );
  }

  u32 k = 0;
  u32 code = 0;
  u32 next_code[16] = {0};
  for (u32 i = 1; i < 16; ++i)
  {
    next_code[i] = code;
    huffman.first_code[i] = (u16) code;
    huffman.first_symbol[i] = (u16) k;
    code += sizes[i];
    if (sizes[i])
    {
      ERROR_ASSERT(code - 1 < (u32) (1 << i), err, IMAGE_ERROR_BAD_CODE_LENGTH, );
    }
    huffman.max_code[i] = code << (16 - i);
    code <<= 1;
    k += sizes[i];
  }
  huffman.max_code[16] = 0x10000;

  mem_set(huffman.fast_table, 0, sizeof(huffman.fast_table));
  for (u32 i = 0; i < code_lengths_size; ++i)
  {
    u8 code_length = code_lengths[i];
    if (!code_length)
    {
      continue;
    }

    u32 canonical_pos =
      next_code[code_length] - huffman.first_code[code_length] + huffman.first_symbol[code_length];
    u16 fast_table_val = (u16) ((u32) (code_length << IMAGE_ZLIB_FAST_BITS) | i);
    huffman.length[canonical_pos] = (u8) code_length;
    huffman.value[canonical_pos] = (u16) i;
    if (code_length <= IMAGE_ZLIB_FAST_BITS)
    {
      u32 j = image_bit_reverse((u16) next_code[code_length], code_length);
      while (j < (1 << IMAGE_ZLIB_FAST_BITS))
      {
        huffman.fast_table[j] = fast_table_val;
        j += (1 << code_length);
      }
    }
    ++next_code[code_length];
  }
}

inline static bool image_zlib_eof(ImageZlibContext& zlib_ctx)
{
  return zlib_ctx.data >= zlib_ctx.data_end;
}

static u32 image_zlib_huffman_decode(ImageZlibContext& zlib_ctx, ImageHuffman& huffman, Error& err)
{
  if (zlib_ctx.bits_buffered < 16)
  {
    if (image_zlib_eof(zlib_ctx))
    {
      ERROR_ASSERT(!zlib_ctx.hit_eof_once, err, IMAGE_ERROR_CORRUPT_ZLIB, (u32) -1);
      zlib_ctx.hit_eof_once = true;
      zlib_ctx.bits_buffered += 16;
    }
    else
    {
      image_zlib_fill_bits(zlib_ctx);
    }
  }

  // NOTE: trying to decode it through the fast lookup table
  u16 fast_value = huffman.fast_table[zlib_ctx.bits_buffer & IMAGE_ZLIB_FAST_MASK];
  if (fast_value)
  {
    u16 code_length = fast_value >> 9;
    zlib_ctx.bits_buffer >>= code_length;
    zlib_ctx.bits_buffered -= code_length;
    return fast_value & 511;
  }

  // NOTE: lookup table failed, doing it the slow way
  u32 bits = image_bit_reverse((u16) zlib_ctx.bits_buffer, 16);
  u32 code_length = IMAGE_ZLIB_FAST_BITS + 1;
  while (true)
  {
    if (bits < huffman.max_code[code_length])
    {
      break;
    }
    ++code_length;
  }
  ERROR_ASSERT(code_length < 16, err, IMAGE_ERROR_CORRUPT_ZLIB, (u32) -1);

  u32 value_idx = (bits >> (16 - code_length)) - huffman.first_code[code_length] +
                  huffman.first_symbol[code_length];
  ERROR_ASSERT(
    value_idx < IMAGE_ZLIB_NUM_SYMBOLS_LITERAL_LENGTH,
    err,
    IMAGE_ERROR_CORRUPT_ZLIB,
    (u32) -1
  );
  ERROR_ASSERT(huffman.length[value_idx] == code_length, err, IMAGE_ERROR_CORRUPT_ZLIB, (u32) -1);
  zlib_ctx.bits_buffer >>= code_length;
  zlib_ctx.bits_buffered -= code_length;
  return huffman.value[value_idx];
}

static void image_zlib_huffman_compute_codes(ImageZlibContext& zlib_ctx, Error& err)
{
  static const u8 code_length_indices[19] =
    {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
  Error error = SUCCESS;

  u32 hlit = image_zlib_read_bits(zlib_ctx, 5) + 257;
  u32 hdist = image_zlib_read_bits(zlib_ctx, 5) + 1;
  u32 hclen = image_zlib_read_bits(zlib_ctx, 4) + 4;
  u32 total_iterations = hlit + hdist;

  u8 code_length_sizes[19] = {};
  ImageHuffman code_length_tree = {};
  for (u32 i = 0; i < hclen; ++i)
  {
    u8 code_length = (u8) image_zlib_read_bits(zlib_ctx, 3);
    code_length_sizes[code_length_indices[i]] = code_length;
  }
  image_zlib_huffman_build(code_length_tree, code_length_sizes, 19, error);
  ERROR_ASSERT(error == SUCCESS, err, error, );

  u32 iteration_idx = 0;
  u8 code_lengths[286 + 32 + 137] = {0};
  while (iteration_idx < total_iterations)
  {
    u32 decoded_value = image_zlib_huffman_decode(zlib_ctx, code_length_tree, error);
    ERROR_ASSERT(error == SUCCESS, err, error, );
    ERROR_ASSERT(decoded_value < 19, err, IMAGE_ERROR_BAD_CODE_LENGTH, );
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
        repeat_times = image_zlib_read_bits(zlib_ctx, 2) + 3;
        ERROR_ASSERT(iteration_idx != 0, err, IMAGE_ERROR_BAD_CODE_LENGTH, );
        fill = code_lengths[iteration_idx - 1];
      }
      else if (decoded_value == 17)
      {
        repeat_times = image_zlib_read_bits(zlib_ctx, 3) + 3;
      }
      else
      {
        repeat_times = image_zlib_read_bits(zlib_ctx, 7) + 11;
      }
      ERROR_ASSERT(
        total_iterations - iteration_idx >= repeat_times,
        err,
        IMAGE_ERROR_BAD_CODE_LENGTH,
      );
      mem_set(code_lengths + iteration_idx, fill, repeat_times);
      iteration_idx += repeat_times;
    }
  }
  ERROR_ASSERT(iteration_idx == total_iterations, err, IMAGE_ERROR_BAD_CODE_LENGTH, );
  image_zlib_huffman_build(zlib_ctx.length, code_lengths, hlit, error);
  ERROR_ASSERT(error == SUCCESS, err, error, );
  image_zlib_huffman_build(zlib_ctx.distance, code_lengths + hlit, hdist, error);
  ERROR_ASSERT(error == SUCCESS, err, error, );
}

#define IMAGE_IHDR (('I' << 24) | ('H' << 16) | ('D' << 8) | ('R'))
#define IMAGE_PLTE (('P' << 24) | ('L' << 16) | ('T' << 8) | ('E'))
#define IMAGE_IDAT (('I' << 24) | ('D' << 16) | ('A' << 8) | ('T'))
#define IMAGE_IEND (('I' << 24) | ('E' << 16) | ('N' << 8) | ('D'))
#define IMAGE_MAX_IDAT_CHUNK_SIZE (1u << 30)

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

static void image_zlib_huffman_parse_block(ImageZlibContext& zlib_ctx, Error& err)
{
  Error error = SUCCESS;

  u8* out = zlib_ctx.out;
  while (true)
  {
    u32 decoded_value = image_zlib_huffman_decode(zlib_ctx, zlib_ctx.length, error);
    ERROR_ASSERT(error == SUCCESS, err, error, );
    if (decoded_value == 256)
    {
      zlib_ctx.out = out;
      ERROR_ASSERT(
        !(zlib_ctx.hit_eof_once && zlib_ctx.bits_buffered < 16),
        err,
        IMAGE_ERROR_UNEXPECTED_END,
      );
      return;
    }
    else if (decoded_value < 256)
    {
      *out++ = (u8) decoded_value;
    }
    else
    {
      ERROR_ASSERT(decoded_value < 286, err, IMAGE_ERROR_BAD_HUFFMAN_CODE, );
      decoded_value -= 257;
      u32 len = length_code_to_base_length[decoded_value];
      if (length_code_to_extra_bits[decoded_value])
      {
        len += image_zlib_read_bits(zlib_ctx, length_code_to_extra_bits[decoded_value]);
      }

      decoded_value = image_zlib_huffman_decode(zlib_ctx, zlib_ctx.distance, error);
      ERROR_ASSERT(error == SUCCESS, err, error, );
      ERROR_ASSERT(decoded_value < 30, err, IMAGE_ERROR_BAD_HUFFMAN_CODE, );
      u32 dist = distance_code_to_base_length[decoded_value];
      if (distance_code_to_extra_bits[decoded_value])
      {
        dist += image_zlib_read_bits(zlib_ctx, distance_code_to_extra_bits[decoded_value]);
      }
      ERROR_ASSERT(out - zlib_ctx.out_start >= dist, err, IMAGE_ERROR_BAD_DISTANCE, );

      u8* p = out - dist;
      while (len--)
      {
        *out++ = *p++;
      }
    }
  }
}

enum ImagePngFilterMethod
{
  IMAGE_PNG_FILTER_METHOD_NONE,
  IMAGE_PNG_FILTER_METHOD_SUB,
  IMAGE_PNG_FILTER_METHOD_UP,
  IMAGE_PNG_FILTER_METHOD_AVERAGE,
  IMAGE_PNG_FILTER_METHOD_PAETH,
  IMAGE_PNG_FILTER_METHOD_AVERAGE_FIRST_LINE,
};

static const u8 first_line_filter_method[5] = {
  IMAGE_PNG_FILTER_METHOD_NONE,
  IMAGE_PNG_FILTER_METHOD_SUB,
  IMAGE_PNG_FILTER_METHOD_NONE,
  IMAGE_PNG_FILTER_METHOD_AVERAGE_FIRST_LINE,
  IMAGE_PNG_FILTER_METHOD_SUB,
};

#define RGBA_CHANNELS 4
#define RGBA8_BYTES_PER_PIXEL 4

static i32 image_png_paeth_predictor(i32 a, i32 b, i32 c)
{
  i32 p = a + b - c;
  i32 pa = (i32) abs(p - a);
  i32 pb = (i32) abs(p - b);
  i32 pc = (i32) abs(p - c);
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

static void
image_png_create_rgba8(Image& img, ImageContext& ctx, u8* data, Allocator& allocator, Error& err)
{
  u8 channels = color_format_to_number_of_channels[ctx.color_format];
  u8 byte_depth = ctx.bit_depth == 16 ? 2 : 1;
  u32 data_bytes_per_pixel = byte_depth * channels;
  usize stride = img.width * RGBA8_BYTES_PER_PIXEL;

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  img.data = (u8*) allocator.alloc(img.width * img.height * RGBA8_BYTES_PER_PIXEL);

  // NOTE: (x + 0b111) >> 3 is basically doing ceil(x / 8.0f)
  usize bytes_per_scanline = ((img.width * channels * ctx.bit_depth) + 7) >> 3;
  u8* filter_buffer = (u8*) scratch_arena.allocator.alloc(bytes_per_scanline * 2);

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
    u8 filter_method = *data++;
    ERROR_ASSERT(filter_method < 5, err, IMAGE_ERROR_INVALID_FILTER, );
    if (line_idx == 0)
    {
      filter_method = first_line_filter_method[filter_method];
    }
    usize data_bytes_per_line = data_bytes_per_pixel * width;
    u8* dest = ((u8*) img.data) + (stride * line_idx);

    switch (filter_method)
    {
      case IMAGE_PNG_FILTER_METHOD_NONE:
      {
        mem_copy(curr, data, data_bytes_per_line);
      }
      break;
      case IMAGE_PNG_FILTER_METHOD_SUB:
      {
        mem_copy(curr, data, data_bytes_per_pixel);
        for (u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i)
        {
          curr[i] = (data[i] + curr[i - data_bytes_per_pixel]) & 255;
        }
      }
      break;
      case IMAGE_PNG_FILTER_METHOD_UP:
      {
        for (u32 i = 0; i < data_bytes_per_line; ++i)
        {
          curr[i] = (data[i] + prev[i]) & 255;
        }
      }
      break;
      case IMAGE_PNG_FILTER_METHOD_AVERAGE:
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
      case IMAGE_PNG_FILTER_METHOD_PAETH:
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
      case IMAGE_PNG_FILTER_METHOD_AVERAGE_FIRST_LINE:
      {
        mem_copy(curr, data, data_bytes_per_pixel);
        for (u32 i = data_bytes_per_pixel; i < data_bytes_per_line; ++i)
        {
          curr[i] = (data[i] + curr[i - data_bytes_per_pixel]) & 255;
        }
      }
      break;
    }
    data += data_bytes_per_line;

    if (ctx.bit_depth < 8)
    {
      ASSERT(false, "[TODO] support smaller depths");
    }
    else if (ctx.bit_depth == 8)
    {
      if (channels == RGBA_CHANNELS)
      {
        mem_copy(dest, curr, width * RGBA_CHANNELS);
      }
      else if (channels == 1)
      {
        ASSERT(false, "[TODO] support grayscale");
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
    else if (ctx.bit_depth == 16)
    {
      ASSERT(false, "[TODO] support 16 bit depth");
    }

    u8* temp = curr;
    curr = prev;
    prev = temp;
  }
}

Image Image::from_file(const char* path, Allocator& allocator, Error& err)
{
  Image img = {};
  Error error = SUCCESS;
  ImageContext ctx = {};
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());
  usize file_size;
  void* file = platform::read_entire_file(path, scratch_arena.allocator, file_size, error);
  ERROR_ASSERT(error == SUCCESS, err, error, img);
  if (file == nullptr)
  {
    err = "Image decoding error. Not found.";
    return img;
  }
  ctx.data = (u8*) file;
  ctx.data_end = (u8*) file + file_size;

  // NOTE: check png header
  static const u8 png_header[] = {137, 80, 78, 71, 13, 10, 26, 10};
  for (u32 i = 0; i < 8; ++i)
  {
    ERROR_ASSERT(image_get8(ctx) == png_header[i], err, IMAGE_ERROR_INVALID_HEADER, img);
  }

  bool first = true;
  bool running = true;
  u8* combined_idat_chunks = nullptr;
  usize combined_idat_chunks_size = 0;
  usize combined_idat_chunks_size_limit = 0;
  while (running && ctx.data != ctx.data_end)
  {
    ImagePngChunk chunk = {};
    chunk.length = image_get32be(ctx);
    chunk.type = image_get32be(ctx);

    switch (chunk.type)
    {
      case IMAGE_IHDR:
      {
        ERROR_ASSERT(first, err, IMAGE_ERROR_IHDR_NOT_FIRST, img);
        first = false;
        ERROR_ASSERT(chunk.length == 13, err, IMAGE_ERROR_INVALID_IHDR, img);
        img.width = image_get32be(ctx);
        img.height = image_get32be(ctx);
        ERROR_ASSERT(
          img.width < IMAGE_MAX_SIZE && img.width > 0 && img.height < IMAGE_MAX_SIZE &&
            img.height > 0,
          err,
          IMAGE_ERROR_INVALID_IHDR,
          img
        );

        u8 bit_depth = image_get8(ctx);
        ERROR_ASSERT(bit_depth > 0 && bit_depth <= 16, err, IMAGE_ERROR_INVALID_IHDR, img);
        ctx.bit_depth = bit_depth;
        u8 color = image_get8(ctx);
        ERROR_ASSERT(
          color == 0 || color == 2 || color == 3 || color == 4 || color == 6,
          err,
          IMAGE_ERROR_INVALID_IHDR,
          img
        );
        ctx.color_format = (ImagePngColorFormat) color;

        u8 compression = image_get8(ctx);
        ERROR_ASSERT(compression == 0, err, IMAGE_ERROR_INVALID_IHDR, img);
        u8 filter = image_get8(ctx);
        ERROR_ASSERT(filter == 0, err, IMAGE_ERROR_INVALID_IHDR, img);
        ctx.interlaced = image_get8(ctx);
        combined_idat_chunks = (u8*) scratch_arena.allocator.alloc_start();
        combined_idat_chunks_size_limit =
          scratch_arena.allocator.size - scratch_arena.allocator.type_data.arena.offset;
      }
      break;

      case IMAGE_PLTE:
      {
        ASSERT(false, "[TODO] support palettes");
      }
      break;

      case IMAGE_IDAT:
      {
        ERROR_ASSERT(!first, err, IMAGE_ERROR_IHDR_NOT_FIRST, img);
        ERROR_ASSERT(chunk.length <= IMAGE_MAX_IDAT_CHUNK_SIZE, err, IMAGE_ERROR_INVALID_IDAT, img);
        ERROR_ASSERT(
          combined_idat_chunks_size < combined_idat_chunks_size_limit,
          err,
          "Image decoding error. Out of memory.",
          img
        );
        mem_copy(combined_idat_chunks + combined_idat_chunks_size, ctx.data, chunk.length);
        combined_idat_chunks_size += chunk.length;
        ctx.data += chunk.length;
      }
      break;

      case IMAGE_IEND:
      {
        ERROR_ASSERT(!first, err, IMAGE_ERROR_IHDR_NOT_FIRST, img);
        ERROR_ASSERT(combined_idat_chunks, err, IMAGE_ERROR_INVALID_IDAT, img);
        scratch_arena.allocator.alloc_finish(combined_idat_chunks + combined_idat_chunks_size);

        // NOTE: zlib parsing
        ImageZlibContext zlib_ctx = {};
        zlib_ctx.data = combined_idat_chunks;
        zlib_ctx.data_end = combined_idat_chunks + combined_idat_chunks_size;

        { // NOTE: header
          u8 cmf = image_zlib_get8(zlib_ctx);
          u8 cm = cmf & 15;
          u8 flg = image_zlib_get8(zlib_ctx);
          ERROR_ASSERT(image_zlib_data_in_bounds(zlib_ctx), err, IMAGE_ERROR_CORRUPT_ZLIB, img);
          ERROR_ASSERT((u16) (cmf * 256 + flg) % 31 == 0, err, IMAGE_ERROR_CORRUPT_ZLIB, img);
          ERROR_ASSERT(!(flg & 32), err, IMAGE_ERROR_CORRUPT_ZLIB, img);
          ERROR_ASSERT(cm == 8, err, IMAGE_ERROR_CORRUPT_ZLIB, img);
        }

        // NOTE: data
        zlib_ctx.out = zlib_ctx.out_start = (u8*) scratch_arena.allocator.alloc_start();
        zlib_ctx.bits_buffered = 0;
        zlib_ctx.bits_buffer = 0;
        bool final = false;
        u32 type = 0;
        do
        {
          final = image_zlib_read_bits(zlib_ctx, 1);
          type = image_zlib_read_bits(zlib_ctx, 2);
          switch (type)
          {
            case IMAGE_PNG_COMPRESSION_TYPE_UNCOMPRESSED:
            {
              if (zlib_ctx.bits_buffered & 7)
              {
                image_zlib_read_bits(zlib_ctx, zlib_ctx.bits_buffered & 7);
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
                header[k++] = image_zlib_get8(zlib_ctx);
              }
              u16 len = (u16) (header[1] << 8) | header[0];
              u16 nlen = (u16) (header[3] << 8) | header[2];
              ERROR_ASSERT(nlen == (len ^ 0xFFFF), err, IMAGE_ERROR_CORRUPT_ZLIB, img);
              ERROR_ASSERT(
                zlib_ctx.data + len <= zlib_ctx.data_end,
                err,
                IMAGE_ERROR_READ_PAST_BUFFER,
                img
              );
              mem_copy(zlib_ctx.out, zlib_ctx.data, len);
              zlib_ctx.data += len;
              zlib_ctx.out += len;
            }
            break;

            case IMAGE_PNG_COMPRESSION_TYPE_FIXED_HUFFMAN:
            {
              image_zlib_huffman_build(
                zlib_ctx.length,
                image_zlib_fixed_huffman_length_alphabet,
                IMAGE_ZLIB_NUM_SYMBOLS_LITERAL_LENGTH,
                error
              );
              ERROR_ASSERT(error == SUCCESS, err, error, img);
              image_zlib_huffman_build(
                zlib_ctx.distance,
                image_zlib_fixed_huffman_distance_alphabet,
                32,
                error
              );
              ERROR_ASSERT(error == SUCCESS, err, error, img);
              image_zlib_huffman_parse_block(zlib_ctx, error);
              ERROR_ASSERT(error == SUCCESS, err, error, img);
            }
            break;

            case IMAGE_PNG_COMPRESSION_TYPE_DYNAMIC_HUFFMAN:
            {
              image_zlib_huffman_compute_codes(zlib_ctx, error);
              ERROR_ASSERT(error == SUCCESS, err, error, img);
              image_zlib_huffman_parse_block(zlib_ctx, error);
              ERROR_ASSERT(error == SUCCESS, err, error, img);
            }
            break;

            case IMAGE_PNG_COMPRESSION_TYPE_ILLEGAL:
            default:
            {
              ERROR_ASSERT(false, err, IMAGE_ERROR_ILLEGAL_COMPRESSION_TYPE, img);
            }
            break;
          }
        }
        while (!final);
        scratch_arena.allocator.alloc_finish(zlib_ctx.out);

        if (!ctx.interlaced)
        {
          image_png_create_rgba8(img, ctx, zlib_ctx.out_start, allocator, error);
          ERROR_ASSERT(error == SUCCESS, err, error, img);
        }
        else
        {
          ASSERT(false, "[TODO] support interlaced pngs");
        }

        image_get32be(ctx);
        running = false;
      }
      break;

      default:
      {
        ERROR_ASSERT(!first, err, IMAGE_ERROR_IHDR_NOT_FIRST, img);
        ctx.data += chunk.length;
      }
      break;
    }
    // NOTE: skip CRC
    image_get32be(ctx);
  }

  err = SUCCESS;
  return img;
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
  out.width = 2;
  out.height = 2;
  out.data = error_placeholder_data;
  return out;
}
