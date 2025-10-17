#ifndef ARENA_H
#define ARENA_H

#define DEFAULT_ALIGNMENT (2*sizeof(void*))

typedef struct Arena
{
  void* buffer;
  usize offset;
  usize buffer_size;
#ifdef GAME_DEBUG
  b32 allocation_active;
#endif
} Arena;

static void* arena_alloc(Arena* arena, usize size, Error* err,
                         ptrsize alignment = DEFAULT_ALIGNMENT);
static void arena_free_all(Arena* arena);
// NOTE(szulf): this is for allocations that i know are at the top, and i have to do some work before knowing the end size
// TODO(szulf): think of a better name
static void* arena_alloc_start(Arena* arena, ptrsize alignment = DEFAULT_ALIGNMENT);
static void arena_alloc_finish(Arena* arena, usize size, Error* err);

static void mem_zero(void* dest, usize bytes);
static void mem_set(void* dest, usize bytes, u8 val);
static void mem_copy(void* dest, const void* src, usize bytes);
static b32 mem_compare(const void* val1, const void* val2, usize bytes);
#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL
static u64 mem_hash_fnv_1a(const void* data, usize size);

#endif
