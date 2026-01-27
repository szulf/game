#ifndef IMAGE_H
#define IMAGE_H

#include "base/base.h"
#include "base/memory.h"

#define IMAGE_MAX_SIZE (1 << 24)

#define IMAGE_ERROR_INVALID_FILTER "Image decoding error. Invalid filter."
#define IMAGE_ERROR_BAD_CODE_LENGTH "Image decoding error. Bad code lengths."
#define IMAGE_ERROR_UNEXPECTED_END "Image decoding error. Unexpected end."
#define IMAGE_ERROR_BAD_HUFFMAN_CODE "Image decoding error. Bad huffman code."
#define IMAGE_ERROR_BAD_DISTANCE "Image decoding error. Bad distance."
#define IMAGE_ERROR_BAD_SIZES "Image decoding error. Bad sizes"
#define IMAGE_ERROR_CORRUPT_ZLIB "Image decoding error. Corrupt Zlib."
#define IMAGE_ERROR_INVALID_HEADER "Image decoding error. Invalid header."
#define IMAGE_ERROR_IHDR_NOT_FIRST "Image decoding error. IHDR is not the first chunk."
#define IMAGE_ERROR_INVALID_IHDR "Image decoding error. Invalid IHDR chunk."
#define IMAGE_ERROR_INVALID_IDAT "Image decoding error. Invalid IDAT chunk."
#define IMAGE_ERROR_READ_PAST_BUFFER "Image decoding error. Read past buffer."
#define IMAGE_ERROR_ILLEGAL_COMPRESSION_TYPE "Image decoding error. Illegal compression type."

struct Image
{
  u8* data;
  usize width;
  usize height;

  static Image from_file(const char* path, Allocator& allocator, Error& out_error);
  static Image error_placeholder();
};

#endif
