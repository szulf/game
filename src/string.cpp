#include "string.h"

const char&
String::operator[](usize idx) const
{
  ASSERT(idx < len, "string bounds exceeded");
  return data[idx];
}

const char*
String::begin() const
{
  return data;
}

const char*
String::end() const
{
  return data + len;
}

String
String::make(const char* cstr)
{
  usize cstr_length = cstr_len(cstr);
  String str;
  str.len = cstr_length;
  str.data = cstr;
  return str;
}

String
String::make(const char* cstr, usize len)
{
  String str;
  str.len = len;
  str.data = cstr;
  return str;
}

usize
String::count_chars(char c) const
{
  usize count = 0;
  for (auto ch : *this)
  {
    if (ch == c) ++count;
  }
  return count;
}

usize
String::find_char(char c, usize start_idx) const
{
  ASSERT(start_idx < len, "start_idx is out of bounds");
  for (usize i = start_idx; i < len; ++i)
  {
    if (data[i] == c) return i;
  }
  return (usize) -1;
}

String
String::prepend(const char* cstr, mem::Arena& arena, Error* err) const
{
  Error error = Error::SUCCESS;
  usize cstr_length = cstr_len(cstr);
  String s;
  s.len = len + cstr_length;
  char* c = (char*) arena.alloc(s.len, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, s);
  mem::copy(c, cstr, cstr_length);
  mem::copy(c + cstr_length, data, len);
  s.data = c;
  return s;
}

// TODO(szulf): this whole implementation is kinda whacky but idc for now
Array<String>
String::split(char c, mem::Arena& arena, Error* err) const
{
  Error error = Error::SUCCESS;
  usize splits_count = count_chars(c) + 1;
  Array<String> splits = Array<String>::make(splits_count, arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, splits);

  usize start_idx = 0;
  for (
    usize found_idx = find_char(c, start_idx);
    found_idx != (usize) -1;
    found_idx = find_char(c, start_idx)
  )
  {
    String s = String::make(data + start_idx, found_idx - start_idx);
    start_idx = found_idx + 1;
    splits.push(s);
    if (start_idx >= len) break;
  }

  if (len - start_idx > 0)
  {
    String s = String::make(data + start_idx, len - start_idx);
    ERROR_ASSERT(error == Error::SUCCESS, *err, error, splits);
    splits.push(s);
  }

  *err = Error::SUCCESS;
  return splits;
}

template <> f32
String::parse(Error* err) const
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
  while (s < data + len)
  {
    if (*s == '.')
    {
      ERROR_ASSERT(!is_fraction, *err, Error::INVALID_PARAMETER, 0.0f);
      is_fraction = true;
      ++s;
    }
    ERROR_ASSERT(*s >= '0' && *s <= '9', *err, Error::INVALID_PARAMETER, 0.0f);
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

  *err = Error::SUCCESS;
  return sign * (val + frac);
}

template <> u32
String::parse(Error* err) const
{
  const char* s = data;

  u32 val = 0;
  while (s < data + len)
  {
    ERROR_ASSERT(*s >= '0' && *s <= '9', *err, Error::INVALID_PARAMETER, (u32) -1);
    val = val * 10u + (u32) (*s - '0');
    ++s;
  }

  *err = Error::SUCCESS;
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
to_string(usize v, mem::Arena& arena)
{
  Error error = Error::SUCCESS;
  usize v_ = v;
  usize v_length = 0;
  while (v_)
  {
    v_ /= 10;
    ++v_length;
  }
  usize out_idx = 0;
  char* out = (char*) arena.alloc(v_length, &error);
  usize pow = upow(10, v_length - 1);
  for (usize i = 0; i < v_length; ++i)
  {
    out[out_idx++] = ((v / pow) % 10) + '0';
    pow /= 10;
  }
  return String::make(out, out_idx);
}

static String
to_string(u32 v, mem::Arena& arena)
{
  return to_string((usize) v, arena);
}

static String
to_string(i32 v, mem::Arena& arena)
{
  Error error = Error::SUCCESS;
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
  char* out = (char*) arena.alloc(v_length, &error);
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
  return String::make(out, out_idx);
}

static String
to_string(const char* v, mem::Arena&)
{
  return String::make(v);
}

static String
to_string(const String& v, mem::Arena&)
{
  return v;
}

static String
to_string(Error v, mem::Arena&)
{
  switch (v)
  {
    case Error::OUT_OF_MEMORY:
    {
      return String::make("out of memory");
    } break;
    case Error::INVALID_PARAMETER:
    {
      return String::make("invalid parameter");
    } break;
    case Error::FILE_READING:
    {
      return String::make("file reading");
    } break;
    case Error::NOT_FOUND:
    {
      return String::make("not found");
    } break;
    case Error::SHADER_COMPILATION:
    {
      return String::make("[SHADER] compilation");
    } break;
    case Error::SHADER_LINKING:
    {
      return String::make("[SHADER] linking");
    } break;
    case Error::PNG_INVALID_HEADER:
    {
      return String::make("[PNG] invalid header");
    } break;
    case Error::PNG_IHDR_NOT_FIRST:
    {
      return String::make("[PNG] ihdr chunk is not first");
    } break;
    case Error::PNG_INVALID_IHDR:
    {
      return String::make("[PNG] invalid ihdr chunk");
    } break;
    case Error::PNG_INVALID_IDAT:
    {
      return String::make("[PNG] invalid idat chunk");
    } break;
    case Error::PNG_BAD_SIZES:
    {
      return String::make("[PNG] bad sizes");
    } break;
    case Error::PNG_BAD_CODE_LENGTHS:
    {
      return String::make("[PNG] bad code lengths");
    } break;
    case Error::PNG_BAD_HUFFMAN_CODE:
    {
      return String::make("[PNG] bad huffman code");
    } break;
    case Error::PNG_BAD_DISTANCE:
    {
      return String::make("[PNG] bad distance");
    } break;
    case Error::PNG_UNEXPECTED_END:
    {
      return String::make("[PNG] unexpected end");
    } break;
    case Error::PNG_CORRUPT_ZLIB:
    {
      return String::make("[PNG] corrupt zlib");
    } break;
    case Error::PNG_READ_PAST_BUFFER:
    {
      return String::make("[PNG] read past buffer");
    } break;
    case Error::PNG_INVALID_FILTER:
    {
      return String::make("[PNG] invalid filter");
    } break;
    case Error::PNG_ILLEGAL_COMPRESION_TYPE:
    {
      return String::make("[PNG] illegal compression type");
    } break;
    case Error::OBJ_INVALID_DATA:
    {
      return String::make("[OBJ] invalid data");
    } break;
    case Error::SUCCESS:
    {
      return String::make("success");
    } break;
  }

  ASSERT(false, "wut");
  return {};
}

static void
format_(mem::Arena&, Error*, char* out, usize& out_idx, const String& fmt, usize& fmt_idx)
{
  while (fmt_idx < fmt.len) out[out_idx++] = fmt[fmt_idx++];
}

template <typename T, typename... Args> static void
format_(mem::Arena& arena, Error* err, char* out, usize& out_idx, const String& fmt, usize& fmt_idx,
        const T& first, const Args&... args)
{
  while (fmt_idx < fmt.len && fmt[fmt_idx] != '{')
  {
    out[out_idx++] = fmt[fmt_idx++];
  }

  if (fmt[fmt_idx] == '{')
  {
    if (fmt_idx + 1 < fmt.len && fmt[++fmt_idx] == '{')
    {
      out[out_idx++] = '{';
      ++fmt_idx;
    }
    else
    {
      ERROR_ASSERT(fmt[fmt_idx] == '}', *err, Error::INVALID_PARAMETER,);
      String formatted = to_string(first, arena);
      mem::copy(out + out_idx, formatted.data, formatted.len);
      ++fmt_idx;
      out_idx += formatted.len;
    }
  }

  format_(arena, err, out, out_idx, fmt, fmt_idx, args...);
}

template <typename... Args> static String
format(mem::Arena& arena, const char* fmt_, const Args&... args)
{
  Error error = Error::SUCCESS;
  char* out = (char*) arena.alloc(1024, &error);
  if (error != Error::SUCCESS) return {};
  String fmt = String::make(fmt_);
  usize out_idx = 0;
  usize fmt_idx = 0;
  format_(arena, &error, out, out_idx, fmt, fmt_idx, args...);
  if (error != Error::SUCCESS) return {};
  return String::make(out, out_idx);
}

