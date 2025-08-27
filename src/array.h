#ifndef ARRAY_H
#define ARRAY_H

template <typename T>
struct Array
{
  usize  capacity;
  usize  len;
  T*     data;
  mem::Arena* arena;

  Array() {}
  Array(mem::Arena& arena, T* data, usize size);

  Result<void> push(const T& val);

  T& operator[](usize idx);
  const T& operator[](usize idx) const;

  T* begin() const;
  T* end() const;
};

#endif
