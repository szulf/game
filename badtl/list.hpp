#ifndef BADTL_LIST_HPP
#define BADTL_LIST_HPP

#include "function.hpp"
#include "utils.hpp"
#include "types.hpp"
#include "allocator.hpp"
#include "memory.hpp"

namespace btl {

template <typename T>
struct List {
  static List<T> make(usize cap, Allocator& allocator);
  static List<T> from_dynamic_arena(Allocator& allocator);

  void push(const T& value);
  void push_range(T* begin, T* end);

  void push_dynamic(const T& value);
  void push_dynamic_range(T* begin, T* end);
  void dynamic_finish();

  T* find(const T& value);
  T* find_reverse(const T& value);
  T* find_fn(Function<bool, const T&> fn);
  T* find_reverse_fn(Function<bool, const T&> fn);

  void pop_to(T* element);
  void clear();

  T& operator[](usize idx);
  const T& operator[](usize idx) const;

  T* begin() const;
  T* end() const;

  T* data;
  usize capacity;
  usize size;
  Allocator* allocator;
  // TODO(szulf): do i want these fields in release builds
  bool dynamic_arena;
  bool dynamic_active;
};

template <typename T>
inline static void write_formatted_type(usize& buf_idx, char* buffer, usize n, const List<T>&);

}

namespace btl {

template <typename T>
List<T> List<T>::make(usize cap, Allocator& allocator) {
  ASSERT(cap != 0, "cannot create an empty list");
  List<T> out = {};
  out.allocator = &allocator;
  out.capacity = cap + 1;
  out.data = static_cast<T*>(allocator.alloc(sizeof(T) * out.capacity));
  return out;
}

template <typename T>
void List<T>::push(const T& value) {
  ASSERT(!dynamic_arena, "cannot push to 'dynamic' arena list");
  if (size + 1 >= capacity) {
    capacity *= 2;
    capacity += 1;
    void* new_data = allocator->alloc(sizeof(T) * capacity);
    btl::mem::copy(new_data, data, size * sizeof(T));
    allocator->free(data);
    data = static_cast<T*>(new_data);
  }
  data[size++] = value;
}

template <typename T>
void List<T>::push_range(T* begin, T* end) {
  ASSERT(!dynamic_arena, "cannot push to 'dynamic' arena list");
  ASSERT(end > begin, "end has to be further than begin");
  if (size + static_cast<usize>(end - begin) >= capacity) {
    while (size + static_cast<usize>(end - begin) >= capacity) {
      capacity *= 2;
    }
    void* new_data = allocator->alloc(sizeof(T) * capacity);
    btl::mem::copy(new_data, data, size * sizeof(T));
    allocator->free(data);
    data = static_cast<T*>(new_data);
  }
  btl::mem::copy(data + size, begin, static_cast<usize>(end - begin) * sizeof(T));
  size += static_cast<usize>(end - begin);
}

template <typename T>
List<T> List<T>::from_dynamic_arena(Allocator& allocator) {
  List<T> out = {};
  out.allocator = &allocator;
  out.data = static_cast<T*>(allocator.start());
  out.dynamic_arena = true;
  out.dynamic_active = true;
  return out;
}

template <typename T>
void List<T>::push_dynamic(const T& value) {
  // NOTE(szulf): these checks take a really long time for some reason
  ASSERT(dynamic_arena, "list has to be a 'dynamic' arena list");
  ASSERT(dynamic_active, "'dynamic' arena has to be active to push");
  ASSERT(
    allocator->type == Allocator::Type::Arena &&
      (size + 1) * sizeof(T) + allocator->type_data.arena.offset < allocator->size,
    "out of memory in 'dynamic' allocation"
  );
  data[size++] = value;
  ++capacity;
}

template <typename T>
void List<T>::push_dynamic_range(T* begin, T* end) {
  ASSERT(dynamic_arena, "list has to be a 'dynamic' arena list");
  ASSERT(dynamic_active, "'dynamic' arena has to be active to push");
  ASSERT(end > begin, "end has to be further than begin");
  usize diff = static_cast<usize>(end - begin);
  ASSERT(
    allocator->type == Allocator::Type::Arena &&
      (size + 1) * sizeof(T) + allocator->type_data.arena.offset < allocator->size,
    "out of memory in 'dynamic' allocation"
  );
  btl::mem::copy(data + size, begin, diff * sizeof(T));
  size += static_cast<usize>(diff);
  capacity += static_cast<usize>(diff);
}

template <typename T>
void List<T>::dynamic_finish() {
  ASSERT(dynamic_arena, "list has to be a 'dynamic' arena list");
  ASSERT(dynamic_active, "'dynamic' arena has to be active to finish");
  allocator->finish(data + size);
  dynamic_active = false;
}

template <typename T>
T* List<T>::find_reverse_fn(Function<bool, const T&> fn) {
  for (i32 i = static_cast<i32>(size) - 1; i >= 0; --i) {
    if (fn(data[i])) {
      return data + i;
    }
  }
  return nullptr;
}

template <typename T>
void List<T>::pop_to(T* element) {
  ASSERT(element >= data && element < data + size, "element has to be within bounds");
  size -= static_cast<usize>((data + size) - element);
#ifdef GAME_DEBUG
  btl::mem::set(element, 0, static_cast<usize>((data + size) - element));
#endif
}

template <typename T>
void List<T>::clear() {
  allocator->free(data);
  size = 0;
  capacity = 0;
}

template <typename T>
T& List<T>::operator[](usize idx) {
  ASSERT(idx < size, "list index out of bounds");
  return data[idx];
}

template <typename T>
const T& List<T>::operator[](usize idx) const {
  ASSERT(idx < size, "list index out of bounds");
  return data[idx];
}

template <typename T>
T* List<T>::begin() const {
  return data;
}

template <typename T>
T* List<T>::end() const {
  ASSERT(!dynamic_arena, "cannot push to 'dynamic' arena list");
  return data + size;
}

template <typename T>
inline static void write_formatted_type(usize& buf_idx, char* buffer, usize n, const List<T>& list) {
  for (const auto& v : list) {
    if (&v != (list.end() - 1)) {
      buf_idx += static_cast<usize>(snprintf(buffer + buf_idx, n - buf_idx, ", "));
    }
    write_formatted_type(buf_idx, buffer, n, v);
  }
}

}

#endif
