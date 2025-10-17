#include "array.h"

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
static Array<T>
array_make(usize cap, Arena* arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  Array<T> arr;
  arr.cap = cap;
  arr.len = 0;
  arr.items = (T*) arena_alloc(arena, sizeof(T) * cap, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, arr);
  return arr;
}

template <typename T>
static void array_push(Array<T>* arr, T val)
{
  ASSERT(arr->len < arr->cap, "array cap exceeded");
  arr->items[arr->len++] = val;
}
