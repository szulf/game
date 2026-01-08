#include "data.h"

namespace data
{

vec3 get_vec3(const String& value, Error& out_error)
{
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  vec3 out = {};

  auto values = value.split(' ', scratch_arena.allocator);
  ERROR_ASSERT(values.size == 3, out_error, ENTITY_READ_ERROR_INVALID_POSITION, out);

  out.x = parse_f32(values[0], error);
  ERROR_ASSERT(error == SUCCESS, out_error, ENTITY_READ_ERROR_INVALID_POSITION, out);
  out.y = parse_f32(values[1], error);
  ERROR_ASSERT(error == SUCCESS, out_error, ENTITY_READ_ERROR_INVALID_POSITION, out);
  out.z = parse_f32(values[2], error);
  ERROR_ASSERT(error == SUCCESS, out_error, ENTITY_READ_ERROR_INVALID_POSITION, out);

  return out;
}

}

#include "gent.cpp"
#include "gkey.cpp"
#include "gscn.cpp"
