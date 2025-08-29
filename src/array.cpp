#include "array.h"

template <typename T>
Array<T>::Array(mem::Arena& arena, T* arr, usize size)
{
  auto data_res = arena.alloc(size * sizeof(T));
  if (data_res.has_error)
  {
    ASSERT(false, "out of memory");
    return;
  }

  data = static_cast<T*>(data_res.val);
  mem::copy(data, arr, size * sizeof(T));
  len = capacity = size;
}

template <typename T>
Array<T>::Array(mem::Arena& arena, usize size)
{
  auto data_res = arena.alloc(size * sizeof(T));
  if (data_res.has_error)
  {
    ASSERT(false, "out of memory");
    return;
  }

  data = static_cast<T*>(data_res.val);
  mem::set(data, 0, size * sizeof(T));
  capacity = size;
  len = 0;
}

template <typename T>
Result<void> Array<T>::push(const T& val)
{
  if (len >= capacity)
  {
    ASSERT(false, "out of memory");
    return {Error::OutOfMemory};
  }

  data[len++] = val;
  return {};
}

template <typename T>
T& Array<T>::operator[](usize idx)
{
  ASSERT(idx >= 0 && idx < len, "idx out of bounds");
  return data[idx];
}

template <typename T>
const T& Array<T>::operator[](usize idx) const
{
  ASSERT(idx >= 0 && idx < len, "idx out of bounds");
  return data[idx];
}

template <typename T>
T* Array<T>::begin() const
{
  return data;
}

template <typename T>
T* Array<T>::end() const
{
  return data + len;
}
