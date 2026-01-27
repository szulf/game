#include "array.h"

template <typename T>
template <typename I>
inline T& Array<T>::operator[](I idx)
{
  ASSERT((usize) idx < size, "list index out of bounds");
  return data[idx];
}

template <typename T>
template <typename I>
inline const T& Array<T>::operator[](I idx) const
{
  ASSERT((usize) idx < size, "list index out of bounds");
  return data[idx];
}

template <typename T>
Array<T> Array<T>::make(ArrayType type, usize cap, Allocator& allocator)
{
  ASSERT(cap != 0, "cannot create an empty list");
  Array<T> out = {};
  out.type = type;
  out.allocator = &allocator;
  out.capacity = cap + 1;
  out.data = (T*) allocator.alloc(sizeof(T) * out.capacity);
  return out;
}

template <typename T>
Array<T> Array<T>::from_dynamic_arena(Allocator& allocator)
{
  Array<T> out = {};
  out.allocator = &allocator;
  out.data = (T*) allocator.alloc_start();
  out.type = ArrayType::DYNAMIC_ARENA;
  out.dynamic_active = true;
  return out;
}

template <typename T>
Array<T> Array<T>::from(T* data, usize size)
{
  Array<T> out = {};
  out.data = data;
  out.capacity = out.size = size;
  out.type = ArrayType::STATIC;
  return out;
}

template <typename T>
void Array<T>::push(const T& value)
{
  switch (type)
  {
    case ArrayType::DYNAMIC:
    {
      if (size + 1 >= capacity)
      {
        capacity *= 2;
        capacity += 1;
        void* new_data = allocator->alloc(sizeof(T) * capacity);
        mem_copy(new_data, data, size * sizeof(T));
        allocator->free(data);
        data = (T*) new_data;
      }
    }
    break;
    case ArrayType::DYNAMIC_ARENA:
    {
      ASSERT(dynamic_active, "'dynamic' arena has to be active to push");
      ASSERT(
        allocator->type == AllocatorType::ARENA &&
          (size + 1) * sizeof(T) + allocator->type_data.arena.offset < allocator->size,
        "out of memory in 'dynamic' allocation"
      );
      ++capacity;
    }
    break;
    case ArrayType::STATIC:
    {
      ASSERT(size + 1 < capacity, "exceeded size of static array");
    }
    break;
  }
  data[size++] = value;
}

template <typename T>
void Array<T>::push_range(const T* begin, const T* end)
{
  ASSERT(end > begin, "end has to be further than begin");
  usize diff = (usize) (end - begin);
  switch (type)
  {
    case ArrayType::DYNAMIC:
    {
      if (size + diff >= capacity)
      {
        while (size + diff >= capacity)
        {
          capacity *= 2;
        }
        void* new_data = allocator->alloc(sizeof(T) * capacity);
        mem_copy(new_data, data, size * sizeof(T));
        free(*allocator, data);
        data = (T*) (new_data);
      }
    }
    break;
    case ArrayType::DYNAMIC_ARENA:
    {
      ASSERT(dynamic_active, "'dynamic' arena has to be active to push");
      ASSERT(
        allocator->type == AllocatorType::ARENA &&
          (size + 1) * sizeof(T) + allocator->type_data.arena.offset < allocator->size,
        "out of memory in 'dynamic' allocation"
      );
      capacity += diff;
    }
    break;
    case ArrayType::STATIC:
    {
      ASSERT(size + diff < capacity, "exceeded size of static array");
    }
    break;
  }
  mem_copy(data + size, begin, diff * sizeof(T));
  size += diff;
}

template <typename T>
void Array<T>::dynamic_finish()
{
  ASSERT(type == ArrayType::DYNAMIC_ARENA, "list has to be a 'dynamic' arena list");
  ASSERT(dynamic_active, "'dynamic' arena has to be active to finish");
  alloc_finish(*allocator, data + size);
  dynamic_active = false;
}

template <typename T>
static i64 quicksort_parition_(Array<T>& arr, i64 lo, i64 hi, bool (*sort_fn)(const T&, const T&))
{
  auto& pivot = arr[hi];
  auto i = lo - 1;

  for (i64 j = lo; j <= hi - 1; ++j)
  {
    if (!sort_fn(arr[j], pivot))
    {
      ++i;
      T temp = arr[i];
      arr[i] = arr[j];
      arr[j] = temp;
    }
  }

  T temp = arr[i + 1];
  arr[i + 1] = arr[hi];
  arr[hi] = temp;
  return i + 1;
}

template <typename T>
static void quicksort_(Array<T>& arr, i64 lo, i64 hi, bool (*sort_fn)(const T&, const T&))
{
  if (lo >= 0 && hi >= 0 && lo < hi)
  {
    auto p = quicksort_parition_(arr, lo, hi, sort_fn);
    quicksort_(arr, lo, p - 1, sort_fn);
    quicksort_(arr, p + 1, hi, sort_fn);
  }
}

template <typename T>
void Array<T>::sort(bool (*sort_fn)(const T&, const T&))
{
  return quicksort_(*this, 0, (i64) (size - 1), sort_fn);
}
