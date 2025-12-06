#ifndef VERTEX_H
#define VERTEX_H

struct Vertex
{
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
};

template <>
usize hash(const Vertex& vertex);
bool operator==(const Vertex& va, const Vertex& vb);

#endif
