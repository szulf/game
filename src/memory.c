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

static void*
arena_alloc(Arena* arena, usize size, Error* err)
{
  return arena_alloc_align(arena, size, DEFAULT_ALIGNMENT, err);
}

static void*
arena_alloc_align(Arena* arena, usize size, ptrsize alignment, Error* err)
{
  ASSERT(arena->buffer != 0, "arena has to be initialized");
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  ASSERT(!arena->allocation_active, "cannot allocate memory when an allocation is active");

  alignment = umin(alignment, 128u);

  u8* curr_addr = (u8*) arena->buffer + arena->offset;
  ptrsize padding = calc_padding(curr_addr, alignment);
  if (arena->offset + padding + size > arena->buffer_size)
  {
    *err = ERROR_OUT_OF_MEMORY;
    return 0;
  }

  arena->offset += padding;

  void* next_addr = curr_addr + padding;
  arena->offset += size;

  mem_zero(next_addr, size);

  *err = ERROR_SUCCESS;
  return next_addr;
}

static void
arena_free_all(Arena* arena)
{
#ifdef GAME_DEBUG
  mem_zero(arena->buffer, arena->buffer_size);
#endif
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
  ASSERT(arena->buffer != 0, "arena has to be initialized");
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  ASSERT(!arena->allocation_active, "cannot start a new allocation when an old one is stil active");

  alignment = umin(alignment, 128u);

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

  if (arena->offset + size > arena->buffer_size)
  {
    *err = ERROR_OUT_OF_MEMORY;
    return;
  }

  *err = ERROR_SUCCESS;
  arena->offset += size;
}

static void
mem_zero(void* dest, usize bytes)
{
  while (bytes--)
  {
    *(u8*)dest++ = 0;
  }
}

static void
mem_set(void* dest, usize bytes, u8 val)
{
  while (bytes--)
  {
    *(u8*)dest++ = val;
  }
}

static void
mem_copy(void* dest, const void* src, usize bytes)
{
  while (bytes--)
  {
    *(u8*)dest++ = *(const u8*)src++;
  }
}

static b32
mem_cmp(const void* v1, const void* v2, usize bytes)
{
  for (usize i = 0; i < bytes; ++i)
  {
    if (((const u8*)v1)[i] != ((const u8*)v2)[i])
    {
      return false;
    }
  }

  return true;
}
