#include "string.h"

bool is_whitespace(char c)
{
  return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}

usize cstr_len(const char* cstr)
{
  const char* s;
  for (s = cstr; *s; ++s)
  {
  }
  return (usize) (s - cstr);
}

force_inline char String::operator[](usize idx) const
{
  ASSERT(idx < size, "string index out of bounds");
  return data[idx];
}

String string_make(const char* cstr)
{
  String s;
  s.data = cstr;
  s.size = cstr_len(cstr);
  return s;
}

String string_make_len(const char* cstr, usize len)
{
  String s;
  s.data = cstr;
  s.size = len;
  return s;
}

usize string_count_chars(const String& str, char c)
{
  usize count = 0;
  for (usize i = 0; i < str.size; ++i)
  {
    if (str[i] == c)
    {
      ++count;
    }
  }
  return count;
}

usize string_find_char(const String& str, char c, usize start_idx)
{
  ASSERT(start_idx < str.size, "start_idx is out of bounds");
  for (usize i = start_idx; i < str.size; ++i)
  {
    if (str[i] == c)
    {
      return i;
    }
  }
  return (usize) -1;
}

bool string_starts_with_cstr(const String& str, const char* cstr)
{
  auto cstr_size = cstr_len(cstr);
  return str.size >= cstr_size && mem_equal(str.data, cstr, cstr_size);
}

bool string_ends_with_cstr(const String& str, const char* cstr)
{
  auto cstr_size = cstr_len(cstr);
  return str.size >= cstr_size && mem_equal(str.data + str.size - cstr_size, cstr, cstr_size);
}

String string_append_cstr(const String& str, const char* cstr, Allocator& allocator)
{
  usize cstr_size = cstr_len(cstr);
  char* out = (char*) alloc(allocator, str.size + cstr_size);
  mem_copy(out, str.data, str.size);
  mem_copy(out + str.size, cstr, cstr_size);
  return string_make_len(out, str.size + cstr_size);
}

String string_append_str(const String& s1, const String& s2, Allocator& allocator)
{
  char* out = (char*) alloc(allocator, s1.size + s2.size);
  mem_copy(out, s1.data, s1.size);
  mem_copy(out + s1.size, s2.data, s2.size);
  return string_make_len(out, s1.size + s2.size);
}

String string_prepend_cstr(const String& str, const char* cstr, Allocator& allocator)
{
  usize cstr_size = cstr_len(cstr);
  char* out = (char*) alloc(allocator, str.size + cstr_size);
  mem_copy(out, cstr, cstr_size);
  mem_copy(out + cstr_size, str.data, str.size);
  return string_make_len(out, str.size + cstr_size);
}

const char* string_to_cstr(const String& str, Allocator& allocator)
{
  char* out = (char*) alloc(allocator, str.size + 1);
  mem_copy(out, str.data, str.size);
  out[str.size] = 0;
  return out;
}

// TODO(szulf): this whole implementation is kinda whacky but idc for now
Array<String> string_split(const String& str, char c, Allocator& allocator)
{
  usize splits_count = string_count_chars(str, c) + 1;
  auto splits = array_make<String>(ARRAY_TYPE_STATIC, splits_count, allocator);

  usize start_idx = 0;
  for (usize found_idx = string_find_char(str, c, start_idx); found_idx != (usize) -1;
       found_idx = string_find_char(str, c, start_idx))
  {
    auto s = string_make_len(str.data + start_idx, found_idx - start_idx);
    start_idx = found_idx + 1;
    if (s.size != 0)
    {
      array_push(splits, s);
    }
    if (start_idx >= str.size)
    {
      break;
    }
  }

  if (str.size - start_idx > 0)
  {
    auto s = string_make_len(str.data + start_idx, str.size - start_idx);
    array_push(splits, s);
  }

  return splits;
}

String string_copy(const String& str, Allocator& allocator)
{
  return string_make_len(string_to_cstr(str, allocator), str.size);
}

f32 string_parse_f32(const String& str, Error& out_error)
{
  const char* s = str.data;

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
  while (s < str.data + str.size)
  {
    if (*s == '.')
    {
      if (is_fraction)
      {
        out_error = GLOBAL_ERROR_INVALID_DATA;
        return 0.0f;
      }
      is_fraction = true;
      ++s;
    }
    if (*s < '0' && *s > '9')
    {
      out_error = GLOBAL_ERROR_INVALID_DATA;
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

u32 string_parse_u32(const String& str, Error& out_error)
{
  const char* s = str.data;

  u32 val = 0;
  while (s < str.data + str.size)
  {
    if (*s < '0' && *s > '9')
    {
      out_error = GLOBAL_ERROR_INVALID_DATA;
      return 0;
    }
    val = val * 10u + static_cast<u32>(*s - '0');
    ++s;
  }

  return val;
}

String string_trim_whitespace(const String& str)
{
  const char* s = str.data;
  while (is_whitespace(*s))
  {
    ++s;
  }

  usize end_size = str.size;
  while (is_whitespace(str[end_size - 1]))
  {
    --end_size;
  }

  return string_make_len(s, str.size - (str.size - end_size) - (usize) (s - str.data));
}

force_inline bool operator==(const String& s1, const String& s2)
{
  return s1.size == s2.size && mem_equal(s1.data, s2.data, s1.size);
}

force_inline bool operator==(const String& s1, const char* cstr)
{
  return s1.size == cstr_len(cstr) && mem_equal(s1.data, cstr, s1.size);
}

force_inline bool operator==(const char* cstr, const String& s1)
{
  return s1.size == cstr_len(cstr) && mem_equal(s1.data, cstr, s1.size);
}

force_inline bool operator!=(const String& sa, const String& sb)
{
  return !(sa == sb);
}

force_inline bool operator!=(const String& str, const char* cstr)
{
  return !(str == cstr);
}

force_inline bool operator!=(const char* cstr, const String& str)
{
  return !(str == cstr);
}

template <>
usize hash<String>(const String& v)
{
  usize hash = 0;
  mem_hash_fnv1(hash, v.data, v.size);
  return hash;
}
