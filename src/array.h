#ifndef ARRAY_H
#define ARRAY_H

template <typename T>
struct Array
{
  T* items;
  usize cap;
  usize len;

  static Array<T> make(usize capacity, Arena* arena, Error* err);
  void push(const T& value);

  T& operator[](usize idx);
  const T& operator[](usize idx) const;
};

#endif
