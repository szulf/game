#include "renderer.h"

bool
Vertex::operator==(const Vertex& other) const
{
  return pos == other.pos && normal == other.normal && uv == other.uv;
}

bool
Vertex::operator!=(const Vertex& other) const
{
  return !(*this == other);
}
