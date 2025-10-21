#include "memory.h"

namespace mem
{

static ptrsize
calc_padding(void* ptr, ptrsize alignment)
{
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");

  ptrsize modulo = (ptrsize) ptr & (alignment - 1);

  if (modulo != 0) return alignment - modulo;
  return 0;
}

void*
Arena::alloc(usize size, Error* err, ptrsize alignment)
{
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  ASSERT(!allocation_active, "cannot allocate memory when an allocation is active");

  u8* curr_addr = (u8*) buffer + offset;
  ptrsize padding = calc_padding(curr_addr, alignment);
  ERROR_ASSERT(offset + padding + size <= buffer_size, *err, Error::OUT_OF_MEMORY, 0);
  offset += padding;
  void* next_addr = curr_addr + padding;
  offset += size;

  *err = Error::SUCCESS;
  return next_addr;
}

void
Arena::free_all()
{
  offset = 0;
}

void*
Arena::alloc_start(ptrsize alignment)
{
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  ASSERT(!allocation_active, "cannot start a new allocation when an old one is stil active");

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

  ERROR_ASSERT(offset + size <= buffer_size, *err, Error::OUT_OF_MEMORY,);
  *err = Error::SUCCESS;
  offset += size;
}

static void
zero(void* dest, usize bytes)
{
  u8* d = (u8*) dest;
  while (bytes--)
  {
    *d++ = 0;
  }
}

static void
set(void* dest, usize bytes, u8 val)
{
  u8* d = (u8*) dest;
  while (bytes--)
  {
    *d++ = val;
  }
}

static void
copy(void* dest, const void* src, usize bytes)
{
  u8* d = (u8*) dest;
  const u8* s = (const u8*) src;
  while (bytes--)
  {
    *d++ = *s++;
  }
}

static b32
compare(const void* v1, const void* v2, usize bytes)
{
  for (usize i = 0; i < bytes; ++i)
  {
    if (((const u8*)v1)[i] != ((const u8*)v2)[i]) return false;
  }
  return true;
}

static u64
hash_fnv_1a(const void* data, usize size)
{
  u64 hash = FNV_OFFSET;
  for (usize i = 0; i < size; ++i)
  {
    hash ^= ((u8*) data)[i];
    hash *= FNV_PRIME;
  }
  return hash;
}

}

