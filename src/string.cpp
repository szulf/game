#include "string.h"

const char&
String::operator[](usize idx) const
{
  ASSERT(idx < len, "string bounds exceeded");
  return data[idx];
}

static String
string_make_cstr(const char* cstr)
{
  usize cstr_length = cstr_len(cstr);
  String str;
  str.len = cstr_length;
  str.data = cstr;
  return str;
}

static String
string_make_cstr_len(const char* cstr, usize len)
{
  String str;
  str.len = len;
  str.data = cstr;
  return str;
}

static usize
string_count_chars(String* str, char c)
{
  usize count = 0;
  for (usize i = 0; i < str->len; ++i)
  {
    if (str->data[i] == c) ++count;
  }
  return count;
}

static usize
string_find_char(String* str, char c, usize start_idx)
{
  ASSERT(start_idx < str->len, "start_idx is out of bounds");
  for (usize i = start_idx; i < str->len; ++i)
  {
    if (str->data[i] == c) return i;
  }
  return (usize) -1;
}

static String
string_prepend(String* str, const char* cstr, Arena* arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  usize cstr_length = cstr_len(cstr);
  String s;
  s.len = str->len + cstr_length;
  char* c = (char*) arena_alloc(arena, s.len, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, s);
  mem_copy(c, cstr, cstr_length);
  mem_copy(c + cstr_length, str->data, str->len);
  s.data = c;
  return s;
}

static void
string_format_(Arena*, Error*, char* out, usize* out_idx, String* fmt, usize* fmt_idx)
{
  while (*fmt_idx < fmt->len) out[(*out_idx)++] = (*fmt)[(*fmt_idx)++];
}

template <typename T, typename... Args> static void
string_format_(Arena* arena, Error* err, char* out, usize* out_idx, String* fmt, usize* fmt_idx,
               const T& first, const Args&... args)
{
  while (*fmt_idx < fmt->len && (*fmt)[*fmt_idx] != '{')
  {
    out[(*out_idx)++] = (*fmt)[(*fmt_idx)++];
  }

  if ((*fmt)[*fmt_idx] == '{')
  {
    if (*fmt_idx + 1 < fmt->len && (*fmt)[++(*fmt_idx)] == '{')
    {
      out[*out_idx++] = '{';
      ++(*fmt_idx);
    }
    else
    {
      ERROR_ASSERT((*fmt)[*fmt_idx] == '}', *err, ERROR_INVALID_PARAMETER,);
      String formatted = to_string(first, arena);
      mem_copy(out + *out_idx, formatted.data, formatted.len);
      ++(*fmt_idx);
      *out_idx += formatted.len;
    }
  }

  string_format_(arena, err, out, out_idx, fmt, fmt_idx, args...);
}

template <typename... Args> static String
string_format(Arena* arena, const char* fmt_, const Args&... args)
{
  static u8 depth = 0;
  ++depth;
  if (depth > 3) return {};
  Error error = ERROR_SUCCESS;
  char* out = (char*) arena_alloc(arena, 1024, &error);
  if (error != ERROR_SUCCESS) return {};
  String fmt = string_make_cstr(fmt_);
  usize out_idx = 0;
  usize fmt_idx = 0;
  string_format_(arena, &error, out, &out_idx, &fmt, &fmt_idx, args...);
  if (error != ERROR_SUCCESS) return {};
  --depth;
  return string_make_cstr_len(out, out_idx);
}

// TODO(szulf): this whole implementation is kinda whacky but idc for now
static Array<String>
string_split(String* str, char c, Arena* arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  usize splits_count = string_count_chars(str, c) + 1;
  Array<String> splits = array_make<String>(splits_count, arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, splits);

  usize start_idx = 0;
  for (
    usize found_idx = string_find_char(str, c, start_idx);
    found_idx != (usize) -1;
    found_idx = string_find_char(str, c, start_idx)
  )
  {
    String s = string_make_cstr_len(str->data + start_idx, found_idx - start_idx);
    start_idx = found_idx + 1;
    array_push(&splits, s);
    if (start_idx >= str->len) break;
  }

  if (str->len - start_idx > 0)
  {
    String s = string_make_cstr_len(str->data + start_idx, str->len - start_idx);
    ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, splits);
    array_push(&splits, s);
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
  else if (*s == '+') ++s;

  b32 is_fraction = false;
  f32 val = 0.0f;
  f32 frac = 0.0f;
  f32 divisor = 1.0f;
  while (s < str->data + str->len)
  {
    if (*s == '.')
    {
      ERROR_ASSERT(!is_fraction, *err, ERROR_INVALID_PARAMETER, 0.0f);
      is_fraction = true;
      ++s;
    }
    ERROR_ASSERT(*s >= '0' && *s <= '9', *err, ERROR_INVALID_PARAMETER, 0.0f);
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

  *err = ERROR_SUCCESS;
  return sign * (val + frac);
}

static u32
string_parse_u32(String* str, Error* err)
{
  const char* s = str->data;

  u32 val = 0;
  while (s < str->data + str->len)
  {
    ERROR_ASSERT(*s >= '0' && *s <= '9', *err, ERROR_INVALID_PARAMETER, (u32) -1);
    val = val * 10u + (u32) (*s - '0');
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

static String
to_string(usize v, Arena* arena)
{
  Error error = ERROR_SUCCESS;
  usize v_ = v;
  usize v_length = 0;
  while (v_)
  {
    v_ /= 10;
    ++v_length;
  }
  usize out_idx = 0;
  char* out = (char*) arena_alloc(arena, v_length, &error);
  usize pow = upow(10, v_length - 1);
  for (usize i = 0; i < v_length; ++i)
  {
    out[out_idx++] = ((v / pow) % 10) + '0';
    pow /= 10;
  }
  return string_make_cstr_len(out, out_idx);
}

static String
to_string(u32 v, Arena* arena)
{
  return to_string((usize) v, arena);
}

static String
to_string(i32 v, Arena* arena)
{
  Error error = ERROR_SUCCESS;
  i32 v_ = v;
  usize v_length = 0;
  if (v < 0)
  {
    ++v_length;
    v_ = -v_;
  }
  while (v_)
  {
    v_ /= 10;
    ++v_length;
  }
  usize out_idx = 0;
  char* out = (char*) arena_alloc(arena, v_length, &error);
  i32 pow = (i32) upow(10, v_length - 1);
  if (v < 0)
  {
    out[out_idx++] = '-';
    --v_length;
  }
  for (usize i = 0; i < v_length; ++i)
  {
    out[out_idx++] = ((v / pow) % 10) + '0';
    pow /= 10;
  }
  return string_make_cstr_len(out, out_idx);
}

static String
to_string(const char* v, Arena*)
{
  return string_make_cstr(v);
}

static String
to_string(const String& v, Arena*)
{
  return v;
}

