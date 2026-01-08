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

enum class AllocatorType
{
  ARENA,
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

  void* alloc_align(usize bytes, usize alignment);
  void* alloc(usize bytes);
  void free(void* ptr);
  void free_all();

  void* alloc_start_align(usize alignment);
  void* alloc_start();
  void alloc_finish(void* end);
};

struct ScratchArena
{
  Allocator& allocator;
  usize start_offset;
  bool top_caller;
  usize idx;

  static ScratchArena get();
  void release();
};

#endif
