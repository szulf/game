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

String String::make(const char* cstr)
{
  String s;
  s.data = cstr;
  s.size = cstr_len(cstr);
  return s;
}

String String::make(const char* cstr, usize len)
{
  String s;
  s.data = cstr;
  s.size = len;
  return s;
}

force_inline char String::operator[](usize idx) const
{
  ASSERT(idx < size, "string index out of bounds");
  return data[idx];
}

usize String::count_chars(char c) const
{
  usize count = 0;
  for (usize i = 0; i < size; ++i)
  {
    if (data[i] == c)
    {
      ++count;
    }
  }
  return count;
}

usize String::find_char(char c, usize start_idx) const
{
  ASSERT(start_idx < size, "start_idx is out of bounds");
  for (usize i = start_idx; i < size; ++i)
  {
    if (data[i] == c)
    {
      return i;
    }
  }
  return (usize) -1;
}

bool String::starts_with(const char* cstr) const
{
  auto cstr_size = cstr_len(cstr);
  return size >= cstr_size && mem_equal(data, cstr, cstr_size);
}

bool String::ends_with(const char* cstr) const
{
  auto cstr_size = cstr_len(cstr);
  return size >= cstr_size && mem_equal(data + size - cstr_size, cstr, cstr_size);
}

String String::append(const char* cstr, Allocator& allocator) const
{
  usize cstr_size = cstr_len(cstr);
  char* out = (char*) allocator.alloc(size + cstr_size);
  mem_copy(out, data, size);
  mem_copy(out + size, cstr, cstr_size);
  return make(out, size + cstr_size);
}

String String::append(const String& str, Allocator& allocator) const
{
  char* out = (char*) allocator.alloc(size + str.size);
  mem_copy(out, data, size);
  mem_copy(out + size, str.data, str.size);
  return make(out, size + str.size);
}

String String::prepend(const char* cstr, Allocator& allocator) const
{
  usize cstr_size = cstr_len(cstr);
  char* out = (char*) allocator.alloc(size + cstr_size);
  mem_copy(out, cstr, cstr_size);
  mem_copy(out + cstr_size, data, size);
  return make(out, size + cstr_size);
}

const char* String::to_cstr(Allocator& allocator) const
{
  char* out = (char*) allocator.alloc(size + 1);
  mem_copy(out, data, size);
  out[size] = 0;
  return out;
}

Array<String> String::split(char c, Allocator& allocator) const
{
  usize splits_count = count_chars(c) + 1;
  auto splits = Array<String>::make(ArrayType::STATIC, splits_count, allocator);

  usize start_idx = 0;
  for (usize found_idx = find_char(c, start_idx); found_idx != (usize) -1;
       found_idx = find_char(c, start_idx))
  {
    auto s = make(data + start_idx, found_idx - start_idx);
    start_idx = found_idx + 1;
    if (s.size != 0)
    {
      splits.push(s);
    }
    if (start_idx >= size)
    {
      break;
    }
  }

  if (size > start_idx)
  {
    auto s = make(data + start_idx, size - start_idx);
    splits.push(s);
  }

  return splits;
}

String String::copy(Allocator& allocator) const
{
  return make(to_cstr(allocator), size);
}

String String::trim_whitespace() const
{
  const char* s = data;
  while (is_whitespace(*s))
  {
    ++s;
  }

  usize end_size = size;
  while (is_whitespace(data[end_size - 1]))
  {
    --end_size;
  }

  return make(s, size - (size - end_size) - (usize) (s - data));
}

f32 parse_f32(const String& str, Error& out_error)
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

u32 parse_u32(const String& str, Error& out_error)
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
