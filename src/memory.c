#include "memory.h"

static ptrsize
calc_padding(void* ptr, ptrsize alignment)
{
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");

  ptrsize modulo = (ptrsize) ptr & (alignment - 1);

  if (modulo != 0)
  {
    return alignment - modulo;
  }
  else
  {
    return 0;
  }
}

static Error
arena_alloc(void** data, Arena* arena, usize size)
{
  return arena_alloc_align(data, arena, size, DEFAULT_ALIGNMENT);
}

static Error
arena_alloc_align(void** data, Arena* arena, usize size, ptrsize alignment)
{
  ASSERT(arena->buffer != 0, "arena has to be initialized");
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");

  alignment = umin(alignment, (ptrsize) 128);

  void* curr_addr = (u8*) arena->buffer + arena->offset;
  ptrsize padding = calc_padding(curr_addr, alignment);
  if (arena->offset + padding + size > arena->buffer_size)
  {
    return OUT_OF_MEMORY;
  }

  arena->offset += padding;

  void* next_addr = (u8*) curr_addr + padding;
  arena->offset += size;

  mem_set(next_addr, 0, size);

  *data = next_addr;

  return SUCCESS;
}

static void
arena_free_all(Arena* arena)
{
#ifdef GAME_DEBUG
  mem_set(arena->buffer, 0, arena->buffer_size);
#endif
  arena->offset = 0;
}

static void
mem_set(void* dest, u8 val, usize bytes)
{
  for (usize i = 0; i < bytes; ++i)
  {
    ((u8*) dest)[i] = val;
  }
}

static void
mem_copy(void* dest, const void* src, usize bytes)
{
  u8* d = dest;
  const u8* s = src;

  for (usize i = 0; i < bytes; ++i)
  {
    d[i] = s[i];
  }
}

// TODO(szulf): idk about this implementation
static bool32
mem_cmp(const void* val1, const void* val2, usize bytes)
{
  const u8* v1 = val1;
  const u8* v2 = val2;

  for (usize i = 0; i < bytes; ++i)
  {
    if (v1[i] != v2[i])
    {
      return false;
    }
  }

  return true;
}
