#include "allocator.hpp"

#include "utils.hpp"
#include "memory.hpp"

namespace btl {

static bool is_power_of_two(usize val) {
  return (val & (val - 1)) == 0;
}

static usize calc_padding(void* ptr, usize alignment) {
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  usize modulo = reinterpret_cast<usize>(ptr) & (alignment - 1);
  if (modulo != 0) {
    return alignment - modulo;
  }
  return 0;
}

Allocator Allocator::make(Type type, usize size) {
  Allocator out = {};
  out.type = type;
  out.size = size;
  out.buffer = mem::alloc(size);
  return out;
}

void* Allocator::alloc(usize alloc_size, usize alignment) {
  switch (type) {
    case Type::Arena: {
      auto& data = type_data.arena;
      ASSERT(!data.dynamic_active, "cannot use allocator when a 'dynamic' allocation is active");
      ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");

      u8* curr_addr = static_cast<u8*>(buffer) + data.offset;
      usize padding = calc_padding(curr_addr, alignment);
      ASSERT(data.offset + padding + alloc_size <= size, "out of memory");
      data.offset += padding;
      void* next_addr = curr_addr + padding;
      data.offset += alloc_size;

      return next_addr;
    } break;
  }

  return nullptr;
}

void Allocator::free(void* ptr) {
  switch (type) {
    case Type::Arena: {
      ASSERT(!type_data.arena.dynamic_active, "cannot use an allocator when a 'dynamic' allocation is active");
    } break;
  }
  (void) ptr;
}

void Allocator::release() {
  switch (type) {
    case Type::Arena: {
      type_data.arena.offset = 0;
    } break;
  }
}

void* Allocator::start(usize alignment) {
  ASSERT(is_power_of_two(alignment), "alignment has to be a power of two");
  ASSERT(type == Type::Arena, "cannot do 'dynamic' allocations with non arena allocators");

  auto& data = type_data.arena;
  ASSERT(!data.dynamic_active, "cannot use allocator when a 'dynamic' allocation is active");

  data.dynamic_active = true;
  u8* curr_addr = static_cast<u8*>(buffer) + data.offset;
  usize padding = calc_padding(curr_addr, alignment);
  data.offset += padding;

  return curr_addr + padding;
}

void Allocator::finish(void* end) {
  ASSERT(type == Type::Arena, "cannot do 'dynamic' allocations with non arena allocators");

  auto& data = type_data.arena;
  ASSERT(data.dynamic_active, "'dynamic' allocation has to be active to finish it");

  u8* curr_addr = static_cast<u8*>(buffer) + data.offset;
  ASSERT(static_cast<u8*>(end) > curr_addr, "invalid pointer provided");
  data.offset += static_cast<usize>(static_cast<u8*>(end) - curr_addr);
  data.dynamic_active = false;
}

thread_local static Allocator scratch_arena = {};
thread_local static bool scratch_arena_initialized = false;
thread_local static bool scratch_arena_top_caller_used = false;

ScratchArena ScratchArena::get() {
  if (!scratch_arena_initialized) {
    // TODO(szulf): maybe in the future somehow dynamically handle how much memory is actually used by the scratch
    // arena?
    scratch_arena = Allocator::make(Allocator::Type::Arena, MB(500));
    scratch_arena_initialized = true;
  }
  bool top_caller = false;
  if (!scratch_arena_top_caller_used) {
    top_caller = true;
    scratch_arena_top_caller_used = true;
  }
  return {scratch_arena, scratch_arena.type_data.arena.offset, top_caller};
}

void ScratchArena::release() {
  // NOTE(szulf): wait until the top caller of ScratchArena::get() releases to actually free any memory
  // this is a way to both reduce mistakes(skill issues?), and simplify the managment of the scratch arena
  // this probably increases the memory usage by a little,
  // but i dont know how to check if i can actually free the memory on release
  // TODO(szulf): maybe think of a better way to handle all this in the future,
  // for now i just want to move on
  if (top_caller) {
#if GAME_DEBUG
    btl::mem::set(
      static_cast<u8*>(scratch_arena.buffer) + start_offset,
      0,
      scratch_arena.type_data.arena.offset - start_offset
    );
#endif
    scratch_arena.type_data.arena.offset = start_offset;
    scratch_arena_top_caller_used = false;
  }
}

}
