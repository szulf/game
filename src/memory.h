#ifndef ARENA_H
#define ARENA_H

namespace mem
{

#define DEFAULT_ALIGNMENT (2*sizeof(void*))

struct Arena
{
  void* buffer;
  usize offset;
  usize buffer_size;

  usize temp_offset;
  bool32 temp_active;

  Result<void*> alloc(usize size, usize alignment = DEFAULT_ALIGNMENT);
  void free_all();

  void start_temp();
  void stop_temp();
};

template <typename T>
void set(T* dest, const T& val, usize amount);

template <typename T>
void copy(T* dest, T* src, usize amount);

}

#endif
