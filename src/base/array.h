#ifndef ARRAY_H
#define ARRAY_H

enum class ArrayType
{
  STATIC,
  DYNAMIC,
  DYNAMIC_ARENA,
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

  static Array<T> make(ArrayType type, usize cap, Allocator& allocator);
  static Array<T> from_dynamic_arena(Allocator& allocator);
  static Array<T> from(T* data, usize size);

  template <typename I>
  inline T& operator[](I idx);
  template <typename I>
  inline const T& operator[](I idx) const;

  void push(const T& value);
  void push_range(const T* begin, const T* end);
  void dynamic_finish();
  void sort(bool (*sort_fn)(const T& a, const T& b));
};

#endif
