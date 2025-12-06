#include "vertex.h"

template <>
usize hash(const Vertex& v)
{
  usize h = U64_FNV_OFFSET;
  mem_hash_fnv1(h, &v.position.x, sizeof(v.position.x));
  mem_hash_fnv1(h, &v.position.y, sizeof(v.position.y));
  mem_hash_fnv1(h, &v.position.z, sizeof(v.position.z));
  mem_hash_fnv1(h, &v.normal.x, sizeof(v.normal.x));
  mem_hash_fnv1(h, &v.normal.y, sizeof(v.normal.y));
  mem_hash_fnv1(h, &v.normal.z, sizeof(v.normal.z));
  mem_hash_fnv1(h, &v.uv.x, sizeof(v.uv.x));
  mem_hash_fnv1(h, &v.uv.y, sizeof(v.uv.y));
  return h;
}

bool operator==(const Vertex& va, const Vertex& vb)
{
  return va.position == vb.position && va.normal == vb.normal && va.uv == vb.uv;
}
