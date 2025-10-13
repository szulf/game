#include "memory.h"

static ptrsize
calc_padding(void* ptr, ptrsize alignment)
{
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");

  ptrsize modulo = (ptrsize) ptr & (alignment - 1);

  if (modulo != 0) return alignment - modulo;
  return 0;
}

static void*
arena_alloc(Arena* arena, usize size, Error* err)
{
  return arena_alloc_align(arena, size, DEFAULT_ALIGNMENT, err);
}

static void*
arena_alloc_align(Arena* arena, usize size, ptrsize alignment, Error* err)
{
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  ASSERT(!arena->allocation_active, "cannot allocate memory when an allocation is active");

  u8* curr_addr = (u8*) arena->buffer + arena->offset;
  ptrsize padding = calc_padding(curr_addr, alignment);
  ERROR_ASSERT(arena->offset + padding + size <= arena->buffer_size, *err, ERROR_OUT_OF_MEMORY, 0);
  arena->offset += padding;
  void* next_addr = curr_addr + padding;
  arena->offset += size;

#ifdef GAME_DEBUG
  mem_zero(next_addr, size);
#endif

  *err = ERROR_SUCCESS;
  return next_addr;
}

static void
arena_free_all(Arena* arena)
{
  arena->offset = 0;
}

static void*
arena_alloc_start(Arena* arena)
{
  return arena_alloc_align_start(arena, DEFAULT_ALIGNMENT);
}

static void*
arena_alloc_align_start(Arena* arena, ptrsize alignment)
{
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  ASSERT(!arena->allocation_active, "cannot start a new allocation when an old one is stil active");

  u8* curr_addr = (u8*) arena->buffer + arena->offset;
  ptrsize padding = calc_padding(curr_addr, alignment);
  arena->offset += padding;
  void* next_addr = curr_addr + padding;

#ifdef GAME_DEBUG
  arena->allocation_active = true;
#endif
  return next_addr;
}

static void
arena_alloc_finish(Arena* arena, usize size, Error* err)
{
  ASSERT(arena->allocation_active, "cannot finish an allocation when it is active");
#ifdef GAME_DEBUG
  arena->allocation_active = false;
#endif

  ERROR_ASSERT(arena->offset + size <= arena->buffer_size, *err, ERROR_OUT_OF_MEMORY,);
  *err = ERROR_SUCCESS;
  arena->offset += size;
}

static void
mem_zero(void* dest, usize bytes)
{
  u8* d = dest;
  while (bytes--)
  {
    *d++ = 0;
  }
}

static void
mem_set(void* dest, usize bytes, u8 val)
{
  u8* d = dest;
  while (bytes--)
  {
    *d++ = val;
  }
}

static void
mem_copy(void* dest, const void* src, usize bytes)
{
  u8* d = dest;
  const u8* s = src;
  while (bytes--)
  {
    *d++ = *s++;
  }
}

static b32
mem_compare(const void* v1, const void* v2, usize bytes)
{
  for (usize i = 0; i < bytes; ++i)
  {
    if (((const u8*)v1)[i] != ((const u8*)v2)[i]) return false;
  }

  return true;
}
