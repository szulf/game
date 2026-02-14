#include "vertex.h"

bool operator==(const Vertex& va, const Vertex& vb)
{
  return va.pos == vb.pos && va.normal == vb.normal && va.uv == vb.uv;
}
