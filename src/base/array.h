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
  bool dynamic_active;

  template <typename I>
  force_inline T& operator[](I idx);
  template <typename I>
  const force_inline T& operator[](I idx) const;
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

template <typename T>
void array_sort(Array<T>& arr, bool (*sort_fn)(const T& a, const T& b));

#endif
