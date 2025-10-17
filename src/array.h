#ifndef ARRAY_H
#define ARRAY_H

template <typename T>
struct Array
{
  usize cap;
  usize len;
  T* items;

  T& operator[](usize idx);
  const T& operator[](usize idx) const;
};

template <typename T>
static Array<T> array_make(usize cap, Arena* arena, Error* err);

template <typename T>
static void array_push(Array<T>* arr, T val);

#endif
