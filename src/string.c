#include "string.h"

static Error
string_init_cstr(String* str, Arena* arena, const char* cstr)
{
  usize cstr_length = cstr_len(cstr);
  char* res;
  Error alloc_error = arena_alloc((void**) &res, arena, cstr_length + 1);
  if (alloc_error != SUCCESS)
  {
    ASSERT(false, "error initializing string");
    return OUT_OF_MEMORY;
  }

  str->data = res;
  mem_copy(str->data, cstr, cstr_length);
  str->cap = cstr_length;
  str->len = cstr_length;
  str->data[str->len] = 0;
  return SUCCESS;
}

static Error
string_init_cstr_len(String* str, Arena* arena, const char* cstr, usize len)
{
  char* res;
  Error alloc_err = arena_alloc((void**) &res, arena, len + 1);
  if (alloc_err != SUCCESS)
  {
    ASSERT(false, "error initializing string");
    return OUT_OF_MEMORY;
  }

  str->data = res;
  mem_copy(str->data, cstr, len);
  str->cap = len;
  str->len = len;
  str->data[str->len] = 0;
  return SUCCESS;
}

static Error
string_init_cap(String* str, Arena* arena, usize cap)
{
  char* res;
  Error alloc_err = arena_alloc((void**) &res, arena, cap + 1);
  if (alloc_err != SUCCESS)
  {
    ASSERT(false, "error initializing string");
    return OUT_OF_MEMORY;
  }

  str->data = res;
  str->cap = cap;
  str->len = 0;
  return SUCCESS;
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

static Error
string_find_char(usize* found_idx, String* str, char c, usize start_idx)
{
  ASSERT(start_idx >= 0 && start_idx < str->len, "start_idx is out of bounds");

  for (usize i = start_idx; i < str->len; ++i)
  {
    if (str->data[i] == c)
    {
      *found_idx = i;
      return SUCCESS;
    }
  }

  return NOT_FOUND;
}

// TODO(szulf): this whole implementation is kinda whacky but idc for now
static Error
string_split(StringArray* splits, String* str, Arena* arena, char c)
{
  usize splits_count = string_count_chars(str, c) + 1;
  ARRAY_INIT(splits, arena, splits_count);

  usize start_idx = 0;
  usize found_idx = 0;
  while (string_find_char(&found_idx, str, c, start_idx) == SUCCESS)
  {
    String s = {};
    Error init_error = string_init_cstr_len(&s, arena, str->data + start_idx, found_idx - start_idx);
    if (init_error != SUCCESS)
    {
      return init_error;
    }
    start_idx = found_idx + 1;
    ARRAY_PUSH(splits, s);

    if (start_idx >= str->len)
    {
      break;
    }
  }

  if (str->len - start_idx > 0)
  {
    String s;
    Error init_error = string_init_cstr_len(&s, arena, str->data + start_idx, str->len - start_idx);
    if (init_error != SUCCESS)
    {
      return init_error;
    }
    ARRAY_PUSH(splits, s);
  }

  return SUCCESS;
}

#include <SDL3/SDL.h>

static Error
string_parse_f32(f32* out, String* str)
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


  bool32 is_fraction = false;
  f32 val = 0.0f;
  f32 frac = 0.0f;
  f32 divisor = 1.0f;
  while (*s)
  {
    if (*s == '.')
    {
      if (is_fraction)
      {
        return INVALID_PARAMETER;
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
      return INVALID_PARAMETER;
    }
    ++s;
  }

  *out = sign * (val + frac);
  return SUCCESS;
}

static Error
string_parse_u32(u32* out, String* str)
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
      return INVALID_PARAMETER;
    }
    ++s;
  }

  *out = val;
  return SUCCESS;
}

static usize
cstr_len(const char* cstr)
{
  const char* s;
  for (s = cstr; *s; ++s) {}
  return (usize) (s - cstr);
}
