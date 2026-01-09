#ifndef VERTEX_H
#define VERTEX_H

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec2 uv;
};

template <>
usize hash(const Vertex& vertex);

bool operator==(const Vertex& va, const Vertex& vb);

#endif
