#ifndef ARENA_H
#define ARENA_H

namespace mem
{

constexpr u8 DEFAULT_ALIGNMENT = 2 * sizeof(void*);

struct Arena
{
  void* buffer;
  usize offset;
  usize buffer_size;
#ifdef GAME_DEBUG
  b32 allocation_active;
#endif

  void* alloc(usize size, Error* err, ptrsize alignment = DEFAULT_ALIGNMENT);
  void free_all();

  // NOTE(szulf): this is for allocations that i know are at the top, and i have to do some work before knowing the end size
  // TODO(szulf): think of a better name
  void* alloc_start(ptrsize alignment = DEFAULT_ALIGNMENT);
  void alloc_finish(usize size, Error* err);
};

static void zero(void* dest, usize bytes);
static void set(void* dest, usize bytes, u8 val);
static void copy(void* dest, const void* src, usize bytes);
static b32 compare(const void* val1, const void* val2, usize bytes);
constexpr u64 FNV_OFFSET = 14695981039346656037UL;
constexpr u64 FNV_PRIME = 1099511628211UL;
static u64 hash_fnv_1a(const void* data, usize size);

}

#endif
