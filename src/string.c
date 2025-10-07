#include "string.h"

static String
string_make_cstr(Arena* arena, const char* cstr, Error* err)
{
  Error error = ERROR_SUCCESS;
  String str = {};
  usize cstr_length = cstr_len(cstr);
  char* res = arena_alloc(arena, cstr_length + 1, &error);
  if (error != ERROR_SUCCESS)
  {
    ASSERT(false, "error initializing string");
    *err = error;
    return str;
  }

  str.data = res;
  mem_copy(str.data, cstr, cstr_length);
  str.cap = cstr_length;
  str.len = cstr_length;
  str.data[str.len] = 0;

  *err = ERROR_SUCCESS;
  return str;
}

static String
string_make_cstr_len(Arena* arena, const char* cstr, usize len, Error* err)
{
  Error error = ERROR_SUCCESS;
  String str = {};
  char* res = arena_alloc(arena, len + 1, &error);
  if (error != ERROR_SUCCESS)
  {
    ASSERT(false, "error initializing string");
    *err = error;
    return str;
  }

  str.data = res;
  mem_copy(str.data, cstr, len);
  str.cap = len;
  str.len = len;
  str.data[str.len] = 0;

  *err = ERROR_SUCCESS;
  return str;
}

static String
string_make_cap(Arena* arena, usize cap, Error* err)
{
  Error error = ERROR_SUCCESS;
  String str = {};
  char* res = arena_alloc(arena, cap + 1, &error);
  if (error != ERROR_SUCCESS)
  {
    ASSERT(false, "error initializing string");
    *err = error;
    return str;
  }

  str.data = res;
  str.cap = cap;
  str.len = 0;

  *err = ERROR_SUCCESS;
  return str;
}

static usize
string_count_chars(String* str, char c)
{
  usize count = 0;
  for (usize i = 0; i < str->len; ++i)
  {
    if (str->data[i] == c)
    {
      ++count;
    }
  }
  return count;
}

static usize
string_find_char(String* str, char c, usize start_idx, Error* err)
{
  ASSERT(start_idx < str->len, "start_idx is out of bounds");

  for (usize i = start_idx; i < str->len; ++i)
  {
    if (str->data[i] == c)
    {
      *err = ERROR_SUCCESS;
      return i;
    }
  }

  *err = ERROR_NOT_FOUND;
  return (usize) -1;
}

static usize
string_count_substrings(String* str, const char* substr)
{
  usize count = 0;
  usize substr_len = cstr_len(substr);
  for (usize i = 0; i < str->len - substr_len + 1; ++i)
  {
    if (mem_compare(str->data + i, substr, substr_len))
    {
      ++count;
    }
  }
  return count;
}

// TODO(szulf): does this handle null terminator?
static String
string_prepend(String* str, const char* cstr, Arena* arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  usize cstr_length = cstr_len(cstr);
  String s = string_make_cap(arena, str->len + cstr_length, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, s);
  mem_copy(s.data, cstr, cstr_length);
  mem_copy(s.data + cstr_length, str->data, str->len);
  return s;
}

// TODO(szulf): this whole implementation is kinda whacky but idc for now
static StringArray
string_split(String* str, Arena* arena, char c, Error* err)
{
  Error error = ERROR_SUCCESS;

  StringArray splits = {};
  usize splits_count = string_count_chars(str, c) + 1;

  ARRAY_INIT(&splits, arena, splits_count, &error);
  if (error != ERROR_SUCCESS)
  {
    ASSERT(false, "couldnt init array");
    *err = error;
    return splits;
  }

  usize start_idx = 0;
  for (
    usize found_idx = string_find_char(str, c, start_idx, &error);
    error == ERROR_SUCCESS;
    found_idx = string_find_char(str, c, start_idx, &error)
  )
  {
    Error init_err = ERROR_SUCCESS;
    String s = string_make_cstr_len(arena, str->data + start_idx, found_idx - start_idx, &init_err);
    if (init_err != ERROR_SUCCESS)
    {
      ASSERT(false, "couldnt init string");
      *err = init_err;
      return splits;
    }
    start_idx = found_idx + 1;
    ARRAY_PUSH(&splits, s);

    if (start_idx >= str->len)
    {
      break;
    }
  }

  if (str->len - start_idx > 0)
  {
    String s = string_make_cstr_len(arena, str->data + start_idx, str->len - start_idx, &error);
    if (error != ERROR_SUCCESS)
    {
      ASSERT(false, "couldnt init string");
      *err = error;
      return splits;
    }
    ARRAY_PUSH(&splits, s);
  }

  *err = ERROR_SUCCESS;
  return splits;
}

static f32
string_parse_f32(String* str, Error* err)
{
  const char* s = str->data;

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
        *err = ERROR_INVALID_PARAMETER;
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
      *err = ERROR_INVALID_PARAMETER;
      return 0.0f;
    }
    ++s;
  }

  *err = ERROR_SUCCESS;
  return sign * (val + frac);
}

static u32
string_parse_u32(String* str, Error* err)
{
  const char* s = str->data;

  u32 val = 0;
  while (*s)
  {
    if (*s >= '0' && *s <= '9')
    {
      val = val * 10u + (u32) (*s - '0');
    }
    else
    {
      *err = ERROR_INVALID_PARAMETER;
      return (u32) -1;
    }
    ++s;
  }

  *err = ERROR_SUCCESS;
  return val;
}

static usize
cstr_len(const char* cstr)
{
  const char* s;
  for (s = cstr; *s; ++s) {}
  return (usize) (s - cstr);
}
