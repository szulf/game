#include "serializer.h"

String Serializer::take_source(
  const String& str,
  SourceType source_type,
  Allocator& allocator,
  Error& out_error
)
{
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  Error error = SUCCESS;
  String out = {};
  switch (source_type)
  {
    case SourceType::STRING:
    {
      out = str;
    }
    break;
    case SourceType::FILE:
    {
      out = platform::read_file_to_string(str.to_cstr(scratch_arena.allocator), allocator, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, out);
    }
    break;
  }
  return out;
}

template <>
f32 Serializer::read<f32>(
  const String& str,
  Error& out_error,
  Allocator* allocator,
  SourceType source_type
)
{
  unused(allocator);

  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  String source = take_source(str, source_type, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});

  const char* s = source.data;

  f32 sign = 1.0f;
  if (*s == '-')
  {
    sign = -1.0f;
    ++s;
  }
  else if (*s == '+')
  {
    ++s;
  }

  bool is_fraction = false;
  f32 val = 0.0f;
  f32 frac = 0.0f;
  f32 divisor = 1.0f;
  while (s < source.data + source.size)
  {
    if (*s == '.')
    {
      if (is_fraction)
      {
        out_error = "Invalid f32 string. Too many dots.";
        return 0.0f;
      }
      is_fraction = true;
      ++s;
    }
    if (*s < '0' || *s > '9')
    {
      out_error = "Invalid f32 string. Invalid character found.";
      return 0.0f;
    }
    if (!is_fraction)
    {
      val = val * 10.0f + (*s - '0');
    }
    else
    {
      divisor *= 10.0f;
      frac = frac + (*s - '0') / divisor;
    }
    ++s;
  }

  return sign * (val + frac);
}

template <>
u32 Serializer::read<u32>(
  const String& str,
  Error& out_error,
  Allocator* allocator,
  SourceType source_type
)
{
  unused(allocator);

  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  String source = take_source(str, source_type, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});

  const char* s = source.data;

  u32 val = 0;
  while (s < source.data + source.size)
  {
    if (*s < '0' || *s > '9')
    {
      out_error = "Invalid u32 string. Invalid character found.";
      return 0;
    }
    val = val * 10u + (u32) (*s - '0');
    ++s;
  }

  return val;
}

template <>
bool Serializer::read<bool>(
  const String& str,
  Error& out_error,
  Allocator* allocator,
  SourceType source_type
)
{
  unused(allocator);

  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  String source = take_source(str, source_type, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});

  if (source == "true")
  {
    return true;
  }
  else if (source == "false")
  {
    return false;
  }

  out_error = "Deserialization decoding error. Invalid bool.";
  return {};
}

template <>
vec2 Serializer::read<vec2>(
  const String& str,
  Error& out_error,
  Allocator* allocator,
  SourceType source_type
)
{
  unused(allocator);

  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  String source = take_source(str, source_type, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  vec2 out = {};

  ERROR_ASSERT(
    source[0] == '(' && source[source.size - 1] == ')',
    out_error,
    "Deserialization decoding error. Invalid vec3.",
    out
  );
  source.size -= 2;
  source.data += 1;

  auto values = source.split(',', scratch_arena.allocator);
  ERROR_ASSERT(values.size == 2, out_error, "Deserialization decoding error. Invalid vec2.", out);

  out.x = read<f32>(values[0].trim_whitespace(), error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, out);
  out.y = read<f32>(values[0].trim_whitespace(), error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, out);

  return out;
}

template <>
vec3 Serializer::read<vec3>(
  const String& str,
  Error& out_error,
  Allocator* allocator,
  SourceType source_type
)
{
  unused(allocator);

  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  String source = take_source(str, source_type, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  vec3 out = {};

  ERROR_ASSERT(
    source[0] == '(' && source[source.size - 1] == ')',
    out_error,
    "Deserialization decoding error. Invalid vec3.",
    out
  );
  source.size -= 2;
  source.data += 1;

  auto values = source.split(',', scratch_arena.allocator);
  ERROR_ASSERT(values.size == 3, out_error, "Deserialization decoding error. Invalid vec3.", out);

  out.x = read<f32>(values[0].trim_whitespace(), error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, out);
  out.y = read<f32>(values[1].trim_whitespace(), error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, out);
  out.z = read<f32>(values[2].trim_whitespace(), error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, out);

  return out;
}
