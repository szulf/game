#ifndef ARRAY_H
#define ARRAY_H

enum ArrayType
{
  ARRAY_TYPE_STATIC,
  ARRAY_TYPE_DYNAMIC,
  ARRAY_TYPE_DYNAMIC_ARENA,
};

template <typename T>
struct Array
{
  T* data;
  usize capacity;
  usize size;
  Allocator* allocator;
  ArrayType type;
  // TODO(szulf): do i want these fields in release builds
  bool dynamic_active;

  force_inline T& operator[](usize idx);
  const force_inline T& operator[](usize idx) const;
};

template <typename T>
Array<T> array_make(ArrayType type, usize cap, Allocator& allocator);
template <typename T>
Array<T> array_from_dynamic_arena(Allocator& allocator);
template <typename T>
Array<T> array_from(T* data, usize size);

template <typename T>
void array_push(Array<T>& arr, const T& value);
template <typename T>
void array_push_range(Array<T>& arr, const T* begin, const T* end);
template <typename T>
void array_dynamic_finish(Array<T>& arr);

#endif
