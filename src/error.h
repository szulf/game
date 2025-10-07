#ifndef ERROR_H
#define ERROR_H

typedef enum Error
{
  ERROR_OUT_OF_MEMORY,
  ERROR_INVALID_PARAMETER,
  ERROR_FILE_READING,
  ERROR_SHADER_COMPILATION,
  ERROR_SHADER_LINKING,
  ERROR_NOT_FOUND,

  ERROR_PNG_IHDR_NOT_FIRST,
  ERROR_PNG_INVALID_IHDR,
  ERROR_PNG_INVALID_IDAT,
  ERROR_PNG_BAD_SIZES,
  ERROR_PNG_BAD_CODE_LENGTHS,
  ERROR_PNG_BAD_HUFFMAN_CODE,
  ERROR_PNG_BAD_DIST,
  ERROR_PNG_UNEXPECTED_END,
  ERROR_PNG_CORRUPT_ZLIB,
  ERROR_PNG_READ_PAST_BUFFER,
  ERROR_PNG_INVALID_FILTER,
  ERROR_PNG_ILLEGAL_COMPRESION_TYPE,

  ERROR_OBJ_INVALID_DATA,

  ERROR_SUCCESS,
} Error;

const char* get_error_string(Error err);

// TODO(szulf): get rid of this(change write to my logging)
#ifdef GAME_DEBUG
#  include <unistd.h>
#  define ERROR_ASSERT(expr, err_var, err_val, ret_val) ERROR_ASSERT_1(expr, err_var, err_val, ret_val, __LINE__, __FILE__)
#  define ERROR_ASSERT_1(expr, err_var, err_val, ret_val, line, file) ERROR_ASSERT_2(expr, err_var, err_val, ret_val, line, file)
#  define ERROR_ASSERT_2(expr, err_var, err_val, ret_val, line, file) do { \
if (!(expr)) \
{ \
  write(0, #expr " " #line " " #file "\n", sizeof(#expr " " #line " " #file "\n")); \
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
