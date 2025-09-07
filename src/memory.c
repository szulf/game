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
  ASSERT(err != 0, "please handle the error");
  ASSERT(arena->buffer != 0, "arena has to be initialized");
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");

  alignment = umin(alignment, 128u);

  void* curr_addr = (u8*) arena->buffer + arena->offset;
  ptrsize padding = calc_padding(curr_addr, alignment);
  if (arena->offset + padding + size > arena->buffer_size)
  {
    *err = ERROR_OUT_OF_MEMORY;
    return 0;
  }

  arena->offset += padding;

  void* next_addr = (u8*) curr_addr + padding;
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

static void
mem_zero(void* dest, usize bytes)
{
  while (bytes--)
  {
    *(u8*)dest++ = 0;
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

static bool32
mem_cmp(const void* v1, const void* v2, usize bytes)
{
  ASSERT(bytes > 0, "invalid bytes number");
  while (--bytes && *(const u8*)v1++ == *(const u8*)v2++) {}
  return !(*(const u8*)v1 - *(const u8*)v2);
}
