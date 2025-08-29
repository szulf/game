#include "string.h"

String::String(mem::Arena& arena, const char* c_str)
{
  auto str_len = c_str_len(c_str);
  auto res = arena.alloc(str_len + 1);
  if (res.has_error)
  {
    ASSERT(false, "error initializing string");
    return;
  }

  data = static_cast<char*>(res.val);
  mem::copy(data, c_str, str_len);
  cap = len = str_len;
  data[len] = 0;
}

String::String(mem::Arena& arena, const char* c_str, usize size)
{
  auto res = arena.alloc(size + 1);
  if (res.has_error)
  {
    ASSERT(false, "error initializing string");
    return;
  }

  data = static_cast<char*>(res.val);
  mem::copy(data, c_str, size);
  cap = len = size;
  data[len] = 0;
}

String::String(mem::Arena& arena, usize size)
{
  auto res = arena.alloc(size + 1);
  if (res.has_error)
  {
    ASSERT(false, "error initializing string");
    return;
  }

  data = static_cast<char*>(res.val);
  cap = size;
  len = 0;
}

usize String::count(char c) const
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

Result<usize> String::find(char c, usize start_idx) const
{
  ASSERT(start_idx >= 0 && start_idx < len, "start_idx is out of bounds");

  for (usize i = start_idx; i < len; ++i)
  {
    if (data[i] == c)
    {
      return {i};
    }
  }

  return {Error::NotFound};
}

// TODO(szulf): this whole implementation is kinda whacky but idc for now
Array<String> String::split(mem::Arena& arena, char c) const
{
  usize splits_count = count(c) + 1;
  Array<String> splits{arena, splits_count};
  usize i = 0;
  for (auto new_i = find(c, i); !new_i.has_error; new_i = find(c, i))
  {
    String str{arena, data + i, new_i.val - i};
    i = new_i.val + 1;
    splits.push(str);

    if (i >= len)
    {
      break;
    }
  }

  if (len - i > 0)
  {
    String str{arena, data + i, len - i};
    splits.push(str);
  }

  return splits;
}

template <>
Result<f32> String::parse<f32>() const
{
  auto* str = data;

  f32 sign = 1.0f;
  if (*str == '-')
  {
    sign = -1.0f;
    ++str;
  }
  else if (*str == '+')
  {
    ++str;
  }


  bool is_fraction = false;
  f32 val{};
  f32 frac{};
  f32 divisor = 1.0f;
  while (*str)
  {
    if (*str == '.')
    {
      if (is_fraction)
      {
        return {Error::InvalidParameter};
      }
      is_fraction = true;
      ++str;
    }

    if (*str >= '0' && *str <= '9')
    {
      if (!is_fraction)
      {
        val = val * 10.0f + (*str - '0');
      }
      else
      {
        divisor *= 10.0f;
        frac = frac + (*str - '0') / divisor;
      }
    }
    else
    {
      return {Error::InvalidParameter};
    }
    ++str;
  }

  return {sign * (val + frac)};
}

template <>
Result<u32> String::parse<u32>() const
{
  auto* str = data;

  u32 val{};
  while (*str)
  {
    if (*str >= '0' && *str <= '9')
    {
      val = val * 10u + static_cast<u32>(*str - '0');
    }
    else
    {
      return {Error::InvalidParameter};
    }
    ++str;
  }

  return {val};
}

const char& String::operator[](usize idx) const
{
  ASSERT(idx >= 0 && idx < len, "idx is out of bounds");
  return data[idx];
}

bool String::operator==(const String& other) const
{
  return mem::cmp(data, other.data, len);
}

bool String::operator==(const char* other) const
{
  return mem::cmp(data, other, len);
}

String String::operator+(const String& other) const
{
  (void) other;
  return String{};
}

usize c_str_len(const char* c_str)
{
  usize len = 0;
  while (*c_str++)
  {
    ++len;
  }

  return len;
}
