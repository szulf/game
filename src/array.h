#ifndef ARRAY_H
#define ARRAY_H

template <typename T>
struct Array
{
  usize cap;
  usize len;
  T* items;

  static Array<T> make(usize cap, mem::Arena& arena, Error* err);
  void push(const T& val);

  T& operator[](usize idx);
  const T& operator[](usize idx) const;

  const T* begin() const;
  const T* end() const;
};

#endif
