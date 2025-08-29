#ifndef ARRAY_H
#define ARRAY_H

// TODO(szulf): should i define a copy constructor here
// i guess shallow copies are fine since memory is managed by the arena
template <typename T>
struct Array
{
  usize capacity;
  usize len;
  T*    data;

  Array() : len{0} {}
  Array(mem::Arena& arena, T* data, usize size);
  Array(mem::Arena& arena, usize size);

  Result<void> push(const T& val);

  T& operator[](usize idx);
  const T& operator[](usize idx) const;

  T* begin() const;
  T* end() const;
};

#endif
