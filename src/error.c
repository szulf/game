#include "error.h"

const char*
get_error_string(Error err)
{
  switch (err)
  {
    case ERROR_SUCCESS:
    {
      return "success";
    } break;
    case ERROR_OUT_OF_MEMORY:
    {
      return "out of memory";
    } break;
    case ERROR_INVALID_PARAMETER:
    {
      return "invalid parameter";
    } break;
    case ERROR_FILE_READING:
    {
      return "file reading";
    } break;
    case ERROR_SHADER_COMPILATION:
    {
      return "shader compilation";
    } break;
    case ERROR_SHADER_LINKING:
    {
      return "shader linking";
    } break;
    case ERROR_NOT_FOUND:
    {
      return "not found";
    } break;
    case ERROR_PNG_IHDR_NOT_FIRST:
    {
      return "[PNG] ihdr chunk is not first";
    } break;
    case ERROR_PNG_INVALID_IHDR:
    {
      return "[PNG] invalid ihdr chunk";
    } break;
    case ERROR_PNG_INVALID_IDAT:
    {
      return "[PNG] invalid idat chunk";
    } break;
    case ERROR_PNG_BAD_SIZES:
    {
      return "[PNG] bad sizes";
    } break;
    case ERROR_PNG_BAD_CODE_LENGTHS:
    {
      return "[PNG] bad code_lengths";
    } break;
    case ERROR_PNG_BAD_HUFFMAN_CODE:
    {
      return "[PNG] bad huffman code";
    } break;
    case ERROR_PNG_BAD_DIST:
    {
      return "[PNG] bad dist";
    } break;
    case ERROR_PNG_UNEXPECTED_END:
    {
      return "[PNG] unexpected end";
    } break;
    case ERROR_PNG_CORRUPT_ZLIB:
    {
      return "[PNG] corrupt zlib";
    } break;
    case ERROR_PNG_READ_PAST_BUFFER:
    {
      return "[PNG] read past buffer";
    } break;
    case ERROR_PNG_INVALID_FILTER:
    {
      return "[PNG] invalid filter";
    } break;
    case ERROR_PNG_ILLEGAL_COMPRESION_TYPE:
    {
      return "[PNG] encountered illegal compression type";
    } break;
    case ERROR_OBJ_INVALID_DATA:
    {
      return "[OBJ] invalid data";
    } break;
  }

  return "wut";
}
