#include "string.h"

String
String::make_cstr(const char* cstr, Arena* arena, Error* err)
{
  Error error = Error::Success;
  String str = {};
  usize cstr_length = cstr_len(cstr);
  char* res = (char*) arena->alloc(cstr_length + 1, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, str);

  str.data = res;
  mem_copy(str.data, cstr, cstr_length);
  str.cap = cstr_length;
  str.len = cstr_length;
  str.data[str.len] = 0;

  *err = Error::Success;
  return str;
}

String
String::make_cstr_len(const char* cstr, usize len, Arena* arena, Error* err)
{
  Error error = Error::Success;
  String str = {};
  char* res = (char*) arena->alloc(len + 1, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, str);

  str.data = res;
  mem_copy(str.data, cstr, len);
  str.cap = len;
  str.len = len;
  str.data[str.len] = 0;

  *err = Error::Success;
  return str;
}

String
String::make_capacity(usize cap, Arena* arena, Error* err)
{
  Error error = Error::Success;
  String str = {};
  char* res = (char*) arena->alloc(cap + 1, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, str);

  str.data = res;
  str.cap = cap;
  str.len = 0;

  *err = Error::Success;
  return str;
}

usize
String::count_chars(char c) const
{
  usize count = 0;
  for (usize i = 0; i < len; ++i)
  {
    if (data[i] == c)
    {
      ++count;
    }
  }
  return count;
}

usize
String::find_char(char c, usize start_idx, Error* err) const
{
  ASSERT(start_idx < len, "start_idx is out of bounds");
  for (usize i = start_idx; i < len; ++i)
  {
    if (data[i] == c)
    {
      *err = Error::Success;
      return i;
    }
  }
  *err = Error::NotFound;
  return (usize) -1;
}

usize
String::count_substrings(const char* substr) const
{
  usize count = 0;
  usize substr_len = cstr_len(substr);
  for (usize i = 0; i < len - substr_len + 1; ++i)
  {
    if (mem_compare(data + i, substr, substr_len))
    {
      ++count;
    }
  }
  return count;
}

#include <stdio.h>

String
String::prepend(const char* cstr, Arena* arena, Error* err) const
{
  Error error = Error::Success;
  usize cstr_length = cstr_len(cstr);
  String s = String::make_capacity(len + cstr_length, arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, s);
  mem_copy(s.data, cstr, cstr_length);
  mem_copy(s.data + cstr_length, data, len + 1);
  s.data[s.cap] = 0;
  return s;
}

// TODO(szulf): this whole implementation is kinda whacky but idc for now
Array<String>
String::split(char c, Arena* arena, Error* err) const
{
  Error error = Error::Success;

  usize splits_count = count_chars(c) + 1;
  auto splits = Array<String>::make(splits_count, arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error, splits);

  usize start_idx = 0;
  for (
    usize found_idx = find_char(c, start_idx, &error);
    error == Error::Success;
    found_idx = find_char(c, start_idx, &error)
  )
  {
    Error init_err = Error::Success;
    String s = String::make_cstr_len(data + start_idx, found_idx - start_idx, arena, &init_err);
    ERROR_ASSERT(init_err == Error::Success, *err, init_err, splits);
    start_idx = found_idx + 1;
    splits.push(s);

    if (start_idx >= len) break;
  }

  if (len - start_idx > 0)
  {
    String s = String::make_cstr_len(data + start_idx, len - start_idx, arena, &error);
    ERROR_ASSERT(error == Error::Success, *err, error, splits);
    splits.push(s);
  }

  *err = Error::Success;
  return splits;
}

template <>
f32
String::parse<f32>(Error* err) const
{
  const char* s = data;

  f32 sign = 1.0f;
  if (*s == '-')
  {
    sign = -1.0f;
    ++s;
  }
  else if (*s == '+') ++s;

  b32 is_fraction = false;
  f32 val = 0.0f;
  f32 frac = 0.0f;
  f32 divisor = 1.0f;
  while (*s)
  {
    if (*s == '.')
    {
      if (is_fraction)
      {
        *err = Error::InvalidParameter;
        return 0.0f;
      }
      is_fraction = true;
      ++s;
    }

    if (*s >= '0' && *s <= '9')
    {
      if (!is_fraction)
      {
        val = val * 10.0f + (*s - '0');
      }
      else
      {
        divisor *= 10.0f;
        frac = frac + (*s - '0') / divisor;
      }
    }
    else
    {
      *err = Error::InvalidParameter;
      return 0.0f;
    }
    ++s;
  }

  *err = Error::Success;
  return sign * (val + frac);
}

template <>
u32
String::parse<u32>(Error* err) const
{
  const char* s = data;

  u32 val = 0;
  while (*s)
  {
    if (*s >= '0' && *s <= '9')
    {
      val = val * 10u + (u32) (*s - '0');
    }
    else
    {
      *err = Error::InvalidParameter;
      return (u32) -1;
    }
    ++s;
  }

  *err = Error::Success;
  return val;
}

static usize
cstr_len(const char* cstr)
{
  const char* s;
  for (s = cstr; *s; ++s) {}
  return (usize) (s - cstr);
}
