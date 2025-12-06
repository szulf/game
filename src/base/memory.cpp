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

void* alloc_align(Allocator& allocator, usize bytes, usize alignment)
{
  switch (allocator.type)
  {
    case ALLOCATOR_TYPE_ARENA:
    {
      auto& data = allocator.type_data.arena;
      ASSERT(!data.dynamic_active, "cannot use allocator when a 'dynamic' allocation is active");
      ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");

      u8* curr_addr = (u8*) allocator.buffer + data.offset;
      usize padding = calc_padding(curr_addr, alignment);
      ASSERT(data.offset + padding + bytes <= allocator.size, "out of memory");
      data.offset += padding;
      void* next_addr = curr_addr + padding;
      data.offset += bytes;

      return next_addr;
    }
    break;
  }

  return nullptr;
}

void* alloc(Allocator& allocator, usize bytes)
{
  return alloc_align(allocator, bytes, DEFAULT_ALIGNMENT);
}

void free(Allocator& allocator, void* ptr)
{
  switch (allocator.type)
  {
    case ALLOCATOR_TYPE_ARENA:
    {
      ASSERT(
        !allocator.type_data.arena.dynamic_active,
        "cannot use an allocator when a 'dynamic' allocation is active"
      );
    }
    break;
  }
  unused(ptr);
}

void free_all(Allocator& allocator)
{
  switch (allocator.type)
  {
    case ALLOCATOR_TYPE_ARENA:
    {
      allocator.type_data.arena.offset = 0;
    }
    break;
  }
}

void* alloc_start_align(Allocator& allocator, usize alignment)
{
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  ASSERT(
    allocator.type == ALLOCATOR_TYPE_ARENA,
    "cannot do 'dynamic' allocations with non arena allocators"
  );

  auto& data = allocator.type_data.arena;
  ASSERT(!data.dynamic_active, "cannot use allocator when a 'dynamic' allocation is active");

  data.dynamic_active = true;
  u8* curr_addr = (u8*) allocator.buffer + data.offset;
  usize padding = calc_padding(curr_addr, alignment);
  data.offset += padding;

  return curr_addr + padding;
}

void* alloc_start(Allocator& allocator)
{
  return alloc_start_align(allocator, DEFAULT_ALIGNMENT);
}

void alloc_finish(Allocator& allocator, void* end)
{
  ASSERT(
    allocator.type == ALLOCATOR_TYPE_ARENA,
    "cannot do 'dynamic' allocations with non arena allocators"
  );

  auto& data = allocator.type_data.arena;
  ASSERT(data.dynamic_active, "'dynamic' allocation has to be active to finish it");

  u8* curr_addr = (u8*) allocator.buffer + data.offset;
  ASSERT((u8*) end > curr_addr, "invalid pointer provided");
  data.offset += (usize) ((u8*) end - curr_addr);
  data.dynamic_active = false;
}

thread_local static Allocator scratch_arena_ = {};
thread_local static bool scratch_arena_initialized_ = false;
thread_local static bool scratch_arena_top_caller_used_ = false;

ScratchArena scratch_arena_get()
{
  if (!scratch_arena_initialized_)
  {
    // TODO(szulf): maybe in the future somehow dynamically handle how much memory is actually used
    // by the scratch arena?
    scratch_arena_.size = MB(500);
    scratch_arena_.type = ALLOCATOR_TYPE_ARENA;
    // TODO(szulf): is there some better way to get this memory?
    scratch_arena_.buffer = malloc(scratch_arena_.size);
    scratch_arena_initialized_ = true;
    mem_set(scratch_arena_.buffer, 0, scratch_arena_.size);
  }
  bool top_caller = false;
  if (!scratch_arena_top_caller_used_)
  {
    top_caller = true;
    scratch_arena_top_caller_used_ = true;
  }
  return {scratch_arena_, scratch_arena_.type_data.arena.offset, top_caller};
}

void scratch_arena_release(ScratchArena& sa)
{
  // NOTE(szulf): wait until the top caller of ScratchArena::get() releases to actually free any
  // memory this is a way to both reduce mistakes(skill issues?), and simplify the managment of the
  // scratch arena this probably increases the memory usage by a little, but i dont know how to
  // check if i can actually free the memory on release
  // TODO(szulf): maybe think of a better way to handle all this in the future,
  // for now i just want to move on
  if (sa.top_caller)
  {
#ifdef MODE_DEBUG
    mem_set(
      (u8*) scratch_arena_.buffer + sa.start_offset,
      0,
      scratch_arena_.type_data.arena.offset - sa.start_offset
    );
#endif
    scratch_arena_.type_data.arena.offset = sa.start_offset;
    scratch_arena_top_caller_used_ = false;
  }
}
