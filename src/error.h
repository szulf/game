#ifndef ERROR_H
#define ERROR_H

enum class Error : u8
{
  OUT_OF_MEMORY,
  INVALID_PARAMETER,
  FILE_READING,
  NOT_FOUND,

  SHADER_COMPILATION,
  SHADER_LINKING,

  PNG_INVALID_HEADER,
  PNG_IHDR_NOT_FIRST,
  PNG_INVALID_IHDR,
  PNG_INVALID_IDAT,
  PNG_BAD_SIZES,
  PNG_BAD_CODE_LENGTHS,
  PNG_BAD_HUFFMAN_CODE,
  PNG_BAD_DISTANCE,
  PNG_UNEXPECTED_END,
  PNG_CORRUPT_ZLIB,
  PNG_READ_PAST_BUFFER,
  PNG_INVALID_FILTER,
  PNG_ILLEGAL_COMPRESION_TYPE,

  OBJ_INVALID_DATA,

  SUCCESS,
};

#ifdef GAME_DEBUG
#  define ERROR_ASSERT(expr, err_var, err_val, ret_val) ERROR_ASSERT_1(expr, err_var, err_val, ret_val, __LINE__, __FILE__)
#  define ERROR_ASSERT_1(expr, err_var, err_val, ret_val, line, file) ERROR_ASSERT_2(expr, err_var, err_val, ret_val, line, file)
#  define ERROR_ASSERT_2(expr, err_var, err_val, ret_val, line, file) do { \
if (!(expr)) \
{ \
  LOG(#expr); \
  (err_var) = (err_val); \
  return ret_val; \
} \
} while (0)
#else
#  define ERROR_ASSERT(expr, err_var, err_val, ret_val, line, file) do { \
if (!(expr)) \
{ \
  (err_var) = (err_val); \
  return ret_val; \
} \
} while (0)
#endif

#endif
