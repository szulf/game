#include "memory.h"

void mem_copy(void* dest, const void* src, usize n)
{
  memcpy(dest, src, n);
}

void mem_set(void* dest, u8 value, usize n)
{
  memset(dest, value, n);
}

bool mem_equal(const void* p1, const void* p2, usize n)
{
  return memcmp(p1, p2, n) == 0;
}

void mem_hash_fnv1(usize& out, const void* data, usize n)
{
  if (out == 0)
  {
    out = U64_FNV_OFFSET;
  }
  const u8* bytes = (const u8*) data;
  for (usize i = 0; i < n; ++i)
  {
    out ^= bytes[i];
    out *= U64_FNV_PRIME;
  }
}

static bool is_power_of_two(usize val)
{
  return (val & (val - 1)) == 0;
}

static usize calc_padding(void* ptr, usize alignment)
{
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  usize modulo = (usize) ptr & (alignment - 1);
  if (modulo != 0)
  {
    return alignment - modulo;
  }
  return 0;
}

void* Allocator::alloc_align(usize bytes, usize alignment)
{
  switch (type)
  {
    case AllocatorType::ARENA:
    {
      auto& data = type_data.arena;
      ASSERT(!data.dynamic_active, "cannot use allocator when a 'dynamic' allocation is active");
      ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");

      u8* curr_addr = (u8*) buffer + data.offset;
      usize padding = calc_padding(curr_addr, alignment);
      ASSERT(data.offset + padding + bytes <= size, "out of memory");
      data.offset += padding;
      void* next_addr = curr_addr + padding;
      data.offset += bytes;

      return next_addr;
    }
    break;
  }

  return nullptr;
}

void* Allocator::alloc(usize bytes)
{
  return alloc_align(bytes, DEFAULT_ALIGNMENT);
}

void Allocator::free(void* ptr)
{
  switch (type)
  {
    case AllocatorType::ARENA:
    {
      ASSERT(
        !type_data.arena.dynamic_active,
        "cannot use an allocator when a 'dynamic' allocation is active"
      );
    }
    break;
  }
  unused(ptr);
}

void Allocator::free_all()
{
  switch (type)
  {
    case AllocatorType::ARENA:
    {
      type_data.arena.offset = 0;
    }
    break;
  }
}

void* Allocator::alloc_start_align(usize alignment)
{
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  ASSERT(type == AllocatorType::ARENA, "cannot do 'dynamic' allocations with non arena allocators");

  auto& data = type_data.arena;
  ASSERT(!data.dynamic_active, "cannot use allocator when a 'dynamic' allocation is active");

  data.dynamic_active = true;
  u8* curr_addr = (u8*) buffer + data.offset;
  usize padding = calc_padding(curr_addr, alignment);
  data.offset += padding;

  return curr_addr + padding;
}

void* Allocator::alloc_start()
{
  return alloc_start_align(DEFAULT_ALIGNMENT);
}

void Allocator::alloc_finish(void* end)
{
  ASSERT(type == AllocatorType::ARENA, "cannot do 'dynamic' allocations with non arena allocators");

  auto& data = type_data.arena;
  ASSERT(data.dynamic_active, "'dynamic' allocation has to be active to finish it");

  u8* curr_addr = (u8*) buffer + data.offset;
  ASSERT((u8*) end > curr_addr, "invalid pointer provided");
  data.offset += (usize) ((u8*) end - curr_addr);
  data.dynamic_active = false;
}

struct ScratchArenaManagementInfo
{
  Allocator arena;
  bool initialized;
  bool in_use;
};

#define SCRATCH_ARENA_BUFFER_SIZE MB(50)
#define SCRATCH_ARENA_COUNT 10
#define SCRATCH_ARENA_TOTAL_MEMORY_SIZE (SCRATCH_ARENA_BUFFER_SIZE * SCRATCH_ARENA_COUNT)
thread_local static void* scratch_arenas_memory_pool_ = nullptr;
thread_local static ScratchArenaManagementInfo scratch_arenas_[SCRATCH_ARENA_COUNT] = {};

ScratchArena ScratchArena::get()
{
  static bool first_init = true;

  for (usize i = 0; i < SCRATCH_ARENA_COUNT; ++i)
  {
    auto& scratch = scratch_arenas_[i];
    if (scratch.in_use)
    {
      continue;
    }

    if (!scratch.initialized)
    {
      if (first_init)
      {
        // NOTE(szulf): the malloc here is fine its happing at most once per thread startup
        // i dont really like using libc, but i dont see a cleaner way here
        scratch_arenas_memory_pool_ = malloc(SCRATCH_ARENA_TOTAL_MEMORY_SIZE);
        first_init = false;
      }

      scratch.arena.size = SCRATCH_ARENA_BUFFER_SIZE;
      scratch.arena.type = AllocatorType::ARENA;
      scratch.arena.buffer = (u8*) scratch_arenas_memory_pool_ + SCRATCH_ARENA_BUFFER_SIZE * i;
      mem_set(scratch.arena.buffer, 0, scratch.arena.size);
      scratch.initialized = true;
    }
    scratch.in_use = true;

    return {scratch.arena, 0, true, i};
  }
  ASSERT(false, "no free scratch_arenas");
}

void ScratchArena::release()
{
  auto& scratch = scratch_arenas_[idx];
  scratch.in_use = false;
  mem_set((u8*) scratch.arena.buffer, 0, scratch.arena.type_data.arena.offset);
  scratch.arena.type_data.arena.offset = 0;
}
