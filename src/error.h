#ifndef ERROR_H
#define ERROR_H

enum class Error : std::uint8_t {
  OutOfMemory,
  InvalidParameter,
  FileReading,
  NotFound,

  ShaderCompilation,
  ShaderLinking,

  PngInvalidHeader,
  PngIhdrNotFirst,
  PngInvalidIhdr,
  PngInvalidIdat,
  PngBadSizes,
  PngBadCodeLengths,
  PngBadHuffmanCode,
  PngBadDistance,
  PngUnexpectedEnd,
  PngCorruptZlib,
  PngReadPastBuffer,
  PngInvalidFilter,
  PngIllegalCompresionType,

  ObjInvalidData,

  Success,
};

const char* error_to_error_string[(usize) Error::Success + 1];

// TODO(szulf): change write to my logging
#ifdef GAME_DEBUG
#  define ERROR_ASSERT(expr, err_var, err_val, ret_val) ERROR_ASSERT_1(expr, err_var, err_val, ret_val, __LINE__, __FILE__)
#  define ERROR_ASSERT_1(expr, err_var, err_val, ret_val, line, file) ERROR_ASSERT_2(expr, err_var, err_val, ret_val, line, file)
#  define ERROR_ASSERT_2(expr, err_var, err_val, ret_val, line, file) do { \
if (!(expr)) { \
  std::cout << #expr << " " << #line << " " << #file << '\n'; \
  (err_var) = (err_val); \
  return ret_val; \
} \
} while (0)
#else
#  define ERROR_ASSERT(expr, err_var, err_val, ret_val) do { \
if (!(expr)) { \
  (err_var) = (err_val); \
  return ret_val; \
} \
} while (0)
#endif

#endif
