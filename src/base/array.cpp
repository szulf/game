#include "array.h"

template <typename T>
force_inline T& Array<T>::operator[](usize idx)
{
  ASSERT(idx < size, "list index out of bounds");
  return data[idx];
}

template <typename T>
const force_inline T& Array<T>::operator[](usize idx) const
{
  ASSERT(idx < size, "list index out of bounds");
  return data[idx];
}

template <typename T>
Array<T> array_make(ArrayType type, usize cap, Allocator& allocator)
{
  ASSERT(cap != 0, "cannot create an empty list");
  Array<T> out = {};
  out.type = type;
  out.allocator = &allocator;
  out.capacity = cap + 1;
  out.data = (T*) alloc(allocator, sizeof(T) * out.capacity);
  return out;
}

template <typename T>
Array<T> array_from_dynamic_arena(Allocator& allocator)
{
  Array<T> out = {};
  out.allocator = allocator;
  out.data = (T*) alloc_start(allocator);
  out.type = ARRAY_TYPE_DYNAMIC_ARENA;
  out.dynamic_active = true;
  return out;
}

template <typename T>
Array<T> array_from(T* data, usize size)
{
  Array<T> out = {};
  out.data = data;
  out.capacity = out.size = size;
  out.type = ARRAY_TYPE_STATIC;
  return out;
}

template <typename T>
void array_push(Array<T>& arr, const T& value)
{
  switch (arr.type)
  {
    case ARRAY_TYPE_DYNAMIC:
    {
      if (arr.size + 1 >= arr.capacity)
      {
        arr.capacity *= 2;
        arr.capacity += 1;
        void* new_data = alloc(*arr.allocator, sizeof(T) * arr.capacity);
        mem_copy(new_data, arr.data, arr.size * sizeof(T));
        free(*arr.allocator, arr.data);
        arr.data = (T*) new_data;
      }
    }
    break;
    case ARRAY_TYPE_DYNAMIC_ARENA:
    {
      ASSERT(arr.dynamic_active, "'dynamic' arena has to be active to push");
      ASSERT(
        arr.allocator->type == ALLOCATOR_TYPE_ARENA &&
          (arr.size + 1) * sizeof(T) + arr.allocator->type_data.arena.offset < arr.allocator->size,
        "out of memory in 'dynamic' allocation"
      );
      ++arr.capacity;
    }
    break;
    case ARRAY_TYPE_STATIC:
    {
      ASSERT(arr.size + 1 < arr.capacity, "exceeded size of static array");
    }
    break;
  }
  arr.data[arr.size++] = value;
}

template <typename T>
void array_push_range(Array<T>& arr, const T* begin, const T* end)
{
  ASSERT(end > begin, "end has to be further than begin");
  usize diff = (usize) (end - begin);
  switch (arr.type)
  {
    case ARRAY_TYPE_DYNAMIC:
    {
      if (arr.size + diff >= arr.capacity)
      {
        while (arr.size + diff >= arr.capacity)
        {
          arr.capacity *= 2;
        }
        void* new_data = alloc(arr.allocator, sizeof(T) * arr.capacity);
        mem_copy(new_data, arr.data, arr.size * sizeof(T));
        free(arr.allocator, arr.data);
        arr.data = (T*) (new_data);
      }
    }
    break;
    case ARRAY_TYPE_DYNAMIC_ARENA:
    {
      ASSERT(arr.dynamic_active, "'dynamic' arena has to be active to push");
      ASSERT(
        arr.allocator.type == ALLOCATOR_TYPE_ARENA &&
          (arr.size + 1) * sizeof(T) + arr.allocator.type_data.arena.offset < arr.allocator.size,
        "out of memory in 'dynamic' allocation"
      );
      arr.capacity += diff;
    }
    break;
    case ARRAY_TYPE_STATIC:
    {
      ASSERT(arr.size + diff < arr.capacity, "exceeded size of static array");
    }
    break;
  }
  mem_copy(arr.data + arr.size, begin, diff * sizeof(T));
  arr.size += diff;
}

template <typename T>
void array_dynamic_finish(Array<T>& arr)
{
  ASSERT(arr.type == ARRAY_TYPE_DYNAMIC_ARENA, "list has to be a 'dynamic' arena list");
  ASSERT(arr.dynamic_active, "'dynamic' arena has to be active to finish");
  alloc_finish(arr.allocator, arr.data + arr.size);
  arr.dynamic_active = false;
}
