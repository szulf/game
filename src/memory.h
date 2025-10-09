#ifndef ARENA_H
#define ARENA_H

#define DEFAULT_ALIGNMENT (2*sizeof(void*))

struct Arena
{
  void* buffer;
  usize offset;
  usize buffer_size;
#ifdef GAME_DEBUG
  bool allocation_active;
#endif

  void* alloc(usize size, Error* err, usize alignment = DEFAULT_ALIGNMENT);
  void free_all();

  // NOTE(szulf): this is for allocations that i know are at the top,
  // and i have to do some work before knowing the end size
  // TODO(szulf): think of a better name
  void* alloc_start(usize alignment = DEFAULT_ALIGNMENT);
  void alloc_finish(usize size, Error* err);
};

static void mem_zero(void* dest, usize bytes);
static void mem_set(void* dest, usize bytes, u8 val);
static void mem_copy(void* dest, const void* src, usize bytes);
static bool mem_compare(const void* val1, const void* val2, usize bytes);

#endif
