#ifndef ERROR_H
#define ERROR_H

typedef enum Error
{
  SUCCESS,
  OUT_OF_MEMORY,
  INVALID_PARAMETER,
  FILE_READING,
  SHADER_COMPILATION,
  SHADER_LINKING,
  NOT_FOUND,
} Error;

const char* get_error_string(Error err);

#endif
