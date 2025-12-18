#ifndef MEMORY_H
#define MEMORY_H

#define KB(x) (x * 1024)
#define MB(x) (KB(x) * 1024l)
#define GB(x) (MB(x) * 1024l)

void mem_copy(void* dest, const void* src, usize n);
void mem_set(void* dest, u8 value, usize n);
bool mem_equal(const void* p1, const void* p2, usize n);
void mem_hash_fnv1(usize& out, const void* data, usize n);

#define DEFAULT_ALIGNMENT (2 * sizeof(void*))

enum AllocatorType
{
  ALLOCATOR_TYPE_ARENA,
};

struct ArenaData
{
  usize offset;
  bool dynamic_active;
};

union AllocatorTypeData
{
  ArenaData arena;
};

struct Allocator
{
  void* buffer;
  usize size;
  AllocatorTypeData type_data;
  AllocatorType type;
};

void* alloc_align(Allocator& allocator, usize bytes, usize alignment);
void* alloc(Allocator& allocator, usize bytes);
void free(Allocator& allocator, void* ptr);
void free_all(Allocator& allocator);

void* alloc_start_align(Allocator& allocator, usize alignment);
void* alloc_start(Allocator& allocator);
void alloc_finish(Allocator& allocator, void* end);

struct ScratchArena
{
  Allocator& allocator;
  usize start_offset;
  bool top_caller;
};

ScratchArena scratch_arena_get();
void scratch_arena_release(ScratchArena& scratch_arena);

#endif
