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

void*
Arena::alloc(usize size, Error* err, ptrsize alignment)
{
  ASSERT(buffer != 0, "arena has to be initialized");
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  ASSERT(!allocation_active, "cannot allocate memory when an allocation is active");

  alignment = umin(alignment, 128u);

  u8* curr_addr = (u8*) buffer + offset;
  ptrsize padding = calc_padding(curr_addr, alignment);
  ERROR_ASSERT(offset + padding + size <= buffer_size, *err, Error::OutOfMemory, 0);

  offset += padding;

  void* next_addr = curr_addr + padding;
  offset += size;

  mem_zero(next_addr, size);

  *err = Error::Success;
  return next_addr;
}

void
Arena::free_all()
{
#ifdef GAME_DEBUG
  mem_zero(buffer, buffer_size);
#endif
  offset = 0;
}

void*
Arena::alloc_start(ptrsize alignment)
{
  ASSERT(buffer != 0, "arena has to be initialized");
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  ASSERT(!allocation_active, "cannot start a new allocation when an old one is stil active");

  alignment = umin(alignment, 128u);

  u8* curr_addr = (u8*) buffer + offset;
  ptrsize padding = calc_padding(curr_addr, alignment);

  offset += padding;

  void* next_addr = curr_addr + padding;

#ifdef GAME_DEBUG
  allocation_active = true;
#endif
  return next_addr;
}

void
Arena::alloc_finish(usize size, Error* err)
{
  ASSERT(allocation_active, "cannot finish an allocation when it is active");
#ifdef GAME_DEBUG
  allocation_active = false;
#endif
  ERROR_ASSERT(offset + size <= buffer_size, *err, Error::OutOfMemory,);

  *err = Error::Success;
  offset += size;
}

static void
mem_zero(void* dest, usize bytes)
{
  u8* d = (u8*) dest;
  while (bytes--)
  {
    *d++ = 0;
  }
}

static void
mem_set(void* dest, usize bytes, u8 val)
{
  u8* d = (u8*) dest;
  while (bytes--)
  {
    *d++ = val;
  }
}

static void
mem_copy(void* dest, const void* src, usize bytes)
{
  u8* d = (u8*) dest;
  const u8* s = (const u8*) src;
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
    if (((const u8*)v1)[i] != ((const u8*)v2)[i])
    {
      return false;
    }
  }
  return true;
}

