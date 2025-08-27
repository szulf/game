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

  set(static_cast<u8*>(next_addr), static_cast<u8>(0), size);

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

template <typename T>
void set(T* dest, const T& val, usize amount)
{
  for (usize i = 0; i < amount; ++i)
  {
    dest[i] = val;
  }
}

template <typename T>
void copy(T* dest, T* src, usize amount)
{
  for (usize i = 0; i < amount; ++i)
  {
    dest[i] = src[i];
  }
}

}
