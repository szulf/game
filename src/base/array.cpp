#include "array.h"

template <typename T>
Array<T> array_make(usize cap, Allocator* allocator)
{
  ASSERT(cap != 0, "cannot create an empty list");
  Array<T> out = {};
  out.allocator = allocator;
  out.capacity = cap + 1;
  out.data = (T*) alloc(allocator, sizeof(T) * out.capacity);
  return out;
}

template <typename T>
Array<T> array_from_dynamic_arena(Allocator* allocator)
{
  Array<T> out = {};
  out.allocator = allocator;
  out.data = (T*) alloc_start(allocator);
  out.dynamic_arena = true;
  out.dynamic_active = true;
  return out;
}

template <typename T>
static void array_checks(Array<T>* arr)
{
  if (arr->dynamic_arena)
  {
    ASSERT(arr->dynamic_active, "'dynamic' arena has to be active to push");
    ASSERT(
      arr->allocator->type == ALLOCATOR_TYPE_ARENA &&
        (arr->size + 1) * sizeof(T) + arr->allocator->type_data.arena.offset < arr->allocator->size,
      "out of memory in 'dynamic' allocation"
    );
    ++arr->capacity;
  }
  else
  {
    if (arr->size + 1 >= arr->capacity)
    {
      arr->capacity *= 2;
      arr->capacity += 1;
      void* new_data = alloc(arr->allocator, sizeof(T) * arr->capacity);
      mem_copy(new_data, arr->data, arr->size * sizeof(T));
      free(arr->allocator, arr->data);
      arr->data = (T*) new_data;
    }
  }
}

template <typename T>
void array_push(Array<T>* arr, const T* value)
{
  array_checks(arr);
  arr->data[arr->size++] = *value;
}

template <typename T>
void array_push_rvalue(Array<T>* arr, T value)
{
  array_checks(arr);
  arr->data[arr->size++] = value;
}

template <typename T>
void array_push_range(Array<T>* arr, const T* begin, const T* end)
{
  usize diff = (usize) (end - begin);
  if (arr->dynamic_arena)
  {
    ASSERT(arr->dynamic_active, "'dynamic' arena has to be active to push");
    ASSERT(end > begin, "end has to be further than begin");
    ASSERT(
      arr->allocator->type == ALLOCATOR_TYPE_ARENA &&
        (arr->size + 1) * sizeof(T) + arr->allocator->type_data.arena.offset < arr->allocator->size,
      "out of memory in 'dynamic' allocation"
    );
    arr->capacity += diff;
  }
  else
  {
    ASSERT(end > begin, "end has to be further than begin");
    if (arr->size + diff >= arr->capacity)
    {
      while (arr->size + diff >= arr->capacity)
      {
        arr->capacity *= 2;
      }
      void* new_data = alloc(arr->allocator, sizeof(T) * arr->capacity);
      mem_copy(new_data, arr->data, arr->size * sizeof(T));
      free(arr->allocator, arr->data);
      arr->data = (T*) (new_data);
    }
  }
  mem_copy(arr->data + arr->size, begin, diff * sizeof(T));
  arr->size += diff;
}

template <typename T>
void array_dynamic_finish(Array<T>* arr)
{
  ASSERT(arr->dynamic_arena, "list has to be a 'dynamic' arena list");
  ASSERT(arr->dynamic_active, "'dynamic' arena has to be active to finish");
  alloc_finish(arr->allocator, arr->data + arr->size);
  arr->dynamic_active = false;
}

template <typename T>
void array_pop_to(Array<T>* arr, T* element)
{
  ASSERT(element >= arr->data && element < arr->data + arr->size, "element has to be within bounds");
  arr->size -= (usize) ((arr->data + arr->size) - element);
#ifdef GAME_DEBUG
  mem_set(element, 0, (usize) ((arr->data + arr->size) - element));
#endif
}

template <typename T>
const T* array_get(const Array<T>* arr, usize idx)
{
  ASSERT(idx < arr->size, "list index out of bounds");
  return &arr->data[idx];
}

template <typename T>
T* array_get(Array<T>* arr, usize idx)
{
  ASSERT(idx < arr->size, "list index out of bounds");
  return &arr->data[idx];
}
