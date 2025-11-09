#include "renderer.h"

bool
Vertex::operator==(const Vertex& other) const
{
  return pos == other.pos && normal == other.normal && uv == other.uv;
}

void
Model::rotate(f32 deg, const math::vec3& axis)
{
  rotation = math::quat{deg, axis};
}

template <>
struct std::formatter<Vertex>
{
  template <class T>
  constexpr T::iterator
  parse(T& ctx)
  {
    return ctx.end();
  }

  template <class T>
  T::iterator
  format(const Vertex& v, T& ctx) const
  {
    auto out = ctx.out();
    out = std::format_to(out, "Vertex{{pos: ");
    out = std::formatter<math::vec3>{}.format(v.pos, ctx);
    out = std::format_to(out, ", normal: ");
    out = std::formatter<math::vec3>{}.format(v.normal, ctx);
    out = std::format_to(out, ", uv: ");
    out = std::formatter<math::vec2>{}.format(v.uv, ctx);
    out = std::format_to(out, "}}");
    return out;
  }
};

template <>
struct std::hash<Vertex>
{
  static constexpr u64 FNV_OFFSET = 14695981039346656037UL;
  static constexpr u64 FNV_PRIME = 1099511628211UL;

  std::size_t
  operator()(const Vertex& v) const
  {
    std::size_t hash = FNV_OFFSET;
    for (usize i = 0; i < sizeof(Vertex); ++i)
    {
      hash ^= ((u8*) &v)[i];
      hash *= FNV_PRIME;
    }
    return hash;
  }
};
