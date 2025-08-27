#include "result.h"

template <typename T>
Result<T>::Result(const T& v) : val{v}
{
  has_error = false;
}

template <typename T>
Result<T>& Result<T>::operator=(const T& v)
{
  val = v;
  has_error = false;

  return *this;
}

template <typename T>
Result<T>::Result(Error e) : err{e}
{
  has_error = true;
}

template <typename T>
Result<T>& Result<T>::operator=(Error e)
{
  err = e;
  has_error = true;
}

Result<void>::Result()
{
  has_error = false;
}

Result<void>::Result(Error e) : err{e}
{
  has_error = true;
}

Result<void>& Result<void>::operator=(Error e)
{
  err = e;
  has_error = true;

  return *this;
}

