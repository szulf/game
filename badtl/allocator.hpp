#ifndef BADTL_ALLOCATOR_HPP
#define BADTL_ALLOCATOR_HPP

#include "types.hpp"

namespace btl {

struct Allocator {
  struct ArenaData {
    usize offset;
    // TODO(szulf): do i want this field in release builds?
    bool dynamic_active;
  };
  enum class Type {
    Arena,
  };
  union TypeData {
    ArenaData arena;
  };

  static Allocator make(Type type, usize size);

  void* alloc(usize bytes, usize alignment = DEFAULT_ALIGNMENT);
  void free(void* ptr);
  void release();

  void* start(usize alignment = DEFAULT_ALIGNMENT);
  void finish(void* end);

  void* buffer;
  usize size;
  TypeData type_data;
  Type type;

  static constexpr usize DEFAULT_ALIGNMENT = 2 * sizeof(void*);
};

struct ScratchArena {
  Allocator& allocator;
  usize start_offset;
  bool top_caller;

  static ScratchArena get();
  void release();
};

}

#endif
