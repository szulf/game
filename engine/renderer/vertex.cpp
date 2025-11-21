#include "engine/renderer/vertex.hpp"
#include "badtl/math.hpp"

btl::usize hash(const core::Vertex& v) {
  btl::usize hash = btl::constants<btl::u64>::fnv_offset;
  for (btl::usize i = 0; i < sizeof(v); ++i) {
    hash ^= reinterpret_cast<const btl::u8*>(&v)[i];
    hash *= btl::constants<btl::u64>::fnv_prime;
  }
  return hash;
}
