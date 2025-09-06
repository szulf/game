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
  }

  return "wut";
}
