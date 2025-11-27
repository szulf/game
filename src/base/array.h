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
  b8 dynamic_arena;
  b8 dynamic_active;
};

template <typename T>
Array<T> array_make(usize cap, Allocator* allocator);
template <typename T>
Array<T> array_from_dynamic_arena(Allocator* allocator);
template <typename T>
void array_push(Array<T>* arr, const T* value);
template <typename T>
void array_push_rvalue(Array<T>* arr, T value);
template <typename T>
void array_push_range(Array<T>* arr, const T* begin, const T* end);
template <typename T>
void array_dynamic_finish(Array<T>* arr);
template <typename T>
void array_pop_to(Array<T>* arr, T* element);
template <typename T>
const T* array_get(const Array<T>* arr, usize idx);
template <typename T>
T* array_get(Array<T>* arr, usize idx);

#endif
