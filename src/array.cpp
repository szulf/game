#include "array.h"

template <typename T> Array<T>
Array<T>::make(usize cap, mem::Arena& arena, Error* err)
{
  Error error = Error::SUCCESS;
  Array<T> arr;
  arr.cap = cap;
  arr.len = 0;
  arr.items = (T*) arena.alloc(sizeof(T) * cap, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, arr);
  return arr;
}

template <typename T> void
Array<T>::push(const T& val)
{
  ASSERT(len < cap, "array cap exceeded");
  items[len++] = val;
}

template <typename T> T&
Array<T>::operator[](usize idx)
{
  ASSERT(idx < len, "array bounds exceeded");
  return items[idx];
}

template <typename T> const T&
Array<T>::operator[](usize idx) const
{
  ASSERT(idx < len, "array bounds exceeded");
  return items[idx];
}

template <typename T>
const T* Array<T>::begin() const
{
  return items;
}

template <typename T>
const T* Array<T>::end() const
{
  return items + len;
}
