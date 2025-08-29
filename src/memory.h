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

void set(void* dest, u8 val, usize bytes);

void copy(void* dest, const void* src, usize bytes);

bool cmp(const void* val1, const void* val2, usize bytes);

}

#endif
