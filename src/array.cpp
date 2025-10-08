#include "array.h"

template <typename T> Array<T>
Array<T>::make(usize capacity, Arena* arena, Error* err)
{
  Array<T> arr;
  arr.cap = capacity;
  arr.len = 0;
  arr.items = (T*) arena->alloc(arr.cap * sizeof(T), err);
  return arr;
}

template <typename T> void
Array<T>::push(const T& value)
{
  if (len >= cap)
  {
    ASSERT(false, "array cap exceeded");
  }
  items[len] = value;
  ++len;
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

