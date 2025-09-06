#ifndef ARENA_H
#define ARENA_H

#define DEFAULT_ALIGNMENT (2*sizeof(void*))

typedef struct Arena
{
  void* buffer;
  usize offset;
  usize buffer_size;
} Arena;

static void* arena_alloc(Arena* arena, usize size, Error* err);
static void* arena_alloc_align(Arena* arena, usize size, ptrsize alignment, Error* err);
static void arena_free_all(Arena* arena);

static void mem_zero(void* dest, usize bytes);
static void mem_copy(void* dest, const void* src, usize bytes);
static bool32 mem_cmp(const void* val1, const void* val2, usize bytes);

#endif
