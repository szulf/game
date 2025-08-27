#ifndef RESULT_H
#define RESULT_H

enum class Error : u8
{
  OutOfMemory,
  InvalidPtr,
  FileReadingError,
  ShaderCompilation,
  ShaderLinking,
};

template <typename T>
struct Result
{
  bool32 has_error;
  union
  {
    T val;
    Error err;
  };

  Result(const T& v);
  Result& operator=(const T& v);
  Result(Error e);
  Result& operator=(Error e);
};

template <>
struct Result<void>
{
  bool32 has_error;
  Error err;

  Result();
  Result(Error e);
  Result& operator=(Error e);
};

#endif
