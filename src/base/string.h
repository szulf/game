#ifndef STRING_H
#define STRING_H

#include "base.h"
#include "memory.h"
#include "array.h"
#include "math.h"
#include "map.h"

bool is_whitespace(char c);
usize cstr_len(const char* cstr);

// NOTE: immutable and not null-terminated
struct String
{
  const char* data;
  usize size;

  static String make(const char* cstr);
  static String make(const char* cstr, usize len);

  inline char operator[](usize idx) const
  {
    ASSERT(idx < size, "string index out of bounds");
    return data[idx];
  }

  usize count(char c) const;
  usize count(const char* c) const;
  usize find(char c, usize start_idx = 0) const;
  usize find_last(char c) const;

  bool starts_with(const char* cstr) const;
  bool ends_with(const char* cstr) const;

  String append(const char* cstr, Allocator& allocator) const;
  String append(const String& str, Allocator& allocator) const;
  String prepend(const char* cstr, Allocator& allocator) const;

  String substr(usize start_idx, usize length, Allocator& allocator) const;

  Array<String> split(char c, Allocator& allocator) const;

  String copy(Allocator& allocator) const;

  String trim_whitespace() const;

  // NOTE: removes directories and the extension
  // NOTE: currently only '/' paths are supported
  String get_filename() const;

  const char* to_cstr(Allocator& allocator) const;
};

f32 parse_f32(const String& source, Error& out_error);
u32 parse_u32(const String& source, Error& out_error);
bool parse_bool(const String& source, Error& out_error);
vec2 parse_vec2(const String& source, Error& out_error);
vec3 parse_vec3(const String& source, Error& out_error);

inline bool operator==(const String& s1, const String& s2)
{
  return s1.size == s2.size && mem_equal(s1.data, s2.data, s1.size);
}

inline bool operator==(const String& s1, const char* cstr)
{
  return s1.size == cstr_len(cstr) && mem_equal(s1.data, cstr, s1.size);
}

inline bool operator==(const char* cstr, const String& s1)
{
  return s1.size == cstr_len(cstr) && mem_equal(s1.data, cstr, s1.size);
}

inline bool operator!=(const String& sa, const String& sb)
{
  return !(sa == sb);
}

inline bool operator!=(const String& str, const char* cstr)
{
  return !(str == cstr);
}

inline bool operator!=(const char* cstr, const String& str)
{
  return !(str == cstr);
}

template <>
usize hash(const String& v);

// TODO: maybe implement format myself in the future?
#define fmt(buf, buf_size, format, ...) libc::snprintf(buf, buf_size, format, __VA_ARGS__)

#endif
