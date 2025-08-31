#ifndef ARENA_H
#define ARENA_H

#define DEFAULT_ALIGNMENT (2*sizeof(void*))

typedef struct Arena
{
  void* buffer;
  usize offset;
  usize buffer_size;
} Arena;

static Error arena_alloc(void** data, Arena* arena, usize size);
static Error arena_alloc_align(void** data, Arena* arena, usize size, ptrsize alignment);
static void arena_free_all(Arena* arena);

static void mem_set(void* dest, u8 val, usize bytes);
static void mem_copy(void* dest, const void* src, usize bytes);
static bool32 mem_cmp(const void* val1, const void* val2, usize bytes);

#endif
