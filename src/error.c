#include "error.h"

const char*
get_error_string(Error err)
{
  switch (err)
  {
    case SUCCESS:
    {
      return "success";
    } break;
    case OUT_OF_MEMORY:
    {
      return "out of memory";
    } break;
    case INVALID_PARAMETER:
    {
      return "invalid parameter";
    } break;
    case FILE_READING:
    {
      return "file reading";
    } break;
    case SHADER_COMPILATION:
    {
      return "shader compilation";
    } break;
    case SHADER_LINKING:
    {
      return "shader linking";
    } break;
    case NOT_FOUND:
    {
      return "not found";
    } break;
  }
}
