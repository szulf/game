#pragma once

#include "badtl/allocator.hpp"
#include "badtl/string.hpp"
#include "badtl/types.hpp"
#include "badtl/result.hpp"

namespace core {

static constexpr btl::u32 MAX_SIZE = 1 << 24;

enum class ImageError {
  InvalidFilter,
  BadCodeLength,
  UnexpectedEnd,
  BadHuffmanCode,
  BadDistance,
  BadSizes,
  CorruptZlib,
  InvalidHeader,
  IHDRNotFirst,
  InvalidIHDR,
  InvalidIDAT,
  ReadPastBuffer,
  IllegalCompressionType,
};

struct Image {
  static btl::Result<Image, ImageError> from_file(const char* path, btl::Allocator& allocator);
  static btl::Result<Image, ImageError> from_file(const btl::String& path, btl::Allocator& allocator);
  static Image error_image();

  btl::u8* data;
  btl::usize width;
  btl::usize height;
};

}
