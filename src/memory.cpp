#include "memory.h"

namespace mem
{

static ptrsize calc_padding(void* ptr, ptrsize alignment)
{
  ASSERT(math::is_power_of_two(alignment), "alignment has to be a power of two");

  ptrsize modulo = reinterpret_cast<ptrsize>(ptr) & (alignment - 1);

  if (modulo != 0)
  {
    return alignment - modulo;
  }
  else
  {
    return 0;
  }
}

Result<void*> Arena::alloc(usize size, ptrsize alignment)
{
  ASSERT(buffer != nullptr, "arena has to be initialized");
  ASSERT(math::is_power_of_two(alignment), "alignment has to be a power of two");

  alignment = math::min(alignment, static_cast<ptrsize>(128));

  void* curr_addr = static_cast<u8*>(buffer) + offset;
  auto padding = calc_padding(curr_addr, alignment);
  if (offset + padding + size > buffer_size)
  {
    return {Error::OutOfMemory};
  }

  offset += padding;

  void* next_addr = static_cast<u8*>(curr_addr) + padding;
  offset += size;

  set(next_addr, 0, size);

  return {next_addr};
}

void Arena::free_all()
{
  ASSERT(!temp_active, "cannot free all when temp allocation is active");
  offset = 0;
}

void Arena::start_temp()
{
  ASSERT(!temp_active, "cannot start a temp alloc when its already active");
  temp_offset = offset;
  temp_active = true;
}

void Arena::stop_temp()
{
  ASSERT(temp_active, "cannot stop a temp alloc when its already stopped");
  offset = temp_offset;
  temp_active = false;
}

void set(void* dest, u8 val, usize bytes)
{
  for (usize i = 0; i < bytes; ++i)
  {
    static_cast<u8*>(dest)[i] = val;
  }
}

void copy(void* dest, const void* src, usize bytes)
{
  auto d = static_cast<u8*>(dest);
  auto s = static_cast<const u8*>(src);

  for (usize i = 0; i < bytes; ++i)
  {
    d[i] = s[i];
  }
}

// TODO(szulf): idk about this implementation
bool cmp(const void* val1, const void* val2, usize bytes)
{
  auto v1 = static_cast<const u8*>(val1);
  auto v2 = static_cast<const u8*>(val2);

  for (usize i = 0; i < bytes; ++i)
  {
    if (v1[i] != v2[i])
    {
      return false;
    }
  }

  return true;
}

}
