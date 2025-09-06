#ifndef ERROR_H
#define ERROR_H

typedef enum Error
{
  ERROR_SUCCESS,
  ERROR_OUT_OF_MEMORY,
  ERROR_INVALID_PARAMETER,
  ERROR_FILE_READING,
  ERROR_SHADER_COMPILATION,
  ERROR_SHADER_LINKING,
  ERROR_NOT_FOUND,
} Error;

const char* get_error_string(Error err);

#endif
