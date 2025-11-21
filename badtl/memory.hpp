#ifndef BADTL_MEMORY_HPP
#define BADTL_MEMORY_HPP

#include <cstring>
#include <cstdlib>

#include "types.hpp"

namespace btl {

#define KB(n) (n * 1024)
#define MB(n) (KB(n) * 1024l)
#define GB(n) (MB(n) * 1024ll)

namespace mem {

inline void copy(void* dest, const void* src, usize n);
inline void set(void* dest, u8 value, usize n);
inline bool eql(const void* p1, const void* p2, usize n);
// TODO(szulf): change this to VirtualAlloc or mmap in the future
inline void* alloc(usize size);

}

}

namespace btl {

namespace mem {

inline void copy(void* dest, const void* src, usize n) {
  std::memcpy(dest, src, n);
}

inline void set(void* dest, u8 value, usize n) {
  std::memset(dest, value, n);
}

inline bool eql(const void* p1, const void* p2, usize n) {
  return std::memcmp(p1, p2, n) == 0;
}

inline void* alloc(usize size) {
  return std::malloc(size);
}

}

}

#endif
