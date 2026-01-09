#include "vertex.h"

template <>
usize hash(const Vertex& v)
{
  usize h = U64_FNV_OFFSET;
  mem_hash_fnv1(h, &v.pos.x, sizeof(v.pos.x));
  mem_hash_fnv1(h, &v.pos.y, sizeof(v.pos.y));
  mem_hash_fnv1(h, &v.pos.z, sizeof(v.pos.z));
  mem_hash_fnv1(h, &v.normal.x, sizeof(v.normal.x));
  mem_hash_fnv1(h, &v.normal.y, sizeof(v.normal.y));
  mem_hash_fnv1(h, &v.normal.z, sizeof(v.normal.z));
  mem_hash_fnv1(h, &v.uv.x, sizeof(v.uv.x));
  mem_hash_fnv1(h, &v.uv.y, sizeof(v.uv.y));
  return h;
}

bool operator==(const Vertex& va, const Vertex& vb)
{
  return va.pos == vb.pos && va.normal == vb.normal && va.uv == vb.uv;
}
