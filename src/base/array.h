#ifndef ARRAY_H
#define ARRAY_H

template <typename T>
struct Array
{
  T* data;
  usize capacity;
  usize size;
  Allocator* allocator;
  // TODO(szulf): do i want these fields in release builds
  bool dynamic_arena;
  bool dynamic_active;

  force_inline T& operator[](usize idx);
  const force_inline T& operator[](usize idx) const;
};

template <typename T>
Array<T> array_make(usize cap, Allocator& allocator);
template <typename T>
Array<T> array_from_dynamic_arena(Allocator& allocator);
template <typename T>
void array_push(Array<T>& arr, const T& value);
template <typename T>
void array_push_rvalue(Array<T>& arr, T value);
template <typename T>
void array_push_range(Array<T>& arr, const T* begin, const T* end);
template <typename T>
void array_dynamic_finish(Array<T>& arr);

#endif
