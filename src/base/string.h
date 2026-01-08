#ifndef STRING_H
#define STRING_H

bool is_whitespace(char c);
usize cstr_len(const char* cstr);

// NOTE(szulf): immutable and not null-terminated
struct String
{
  const char* data;
  usize size;

  static String make(const char* cstr);
  static String make(const char* cstr, usize len);

  force_inline char operator[](usize idx) const;

  usize count_chars(char c) const;
  usize find_char(char c, usize start_idx) const;

  bool starts_with(const char* cstr) const;
  bool ends_with(const char* cstr) const;

  String append(const char* cstr, Allocator& allocator) const;
  String append(const String& str, Allocator& allocator) const;
  String prepend(const char* cstr, Allocator& allocator) const;

  Array<String> split(char c, Allocator& allocator) const;

  String copy(Allocator& allocator) const;

  String trim_whitespace() const;

  const char* to_cstr(Allocator& allocator) const;
};

f32 parse_f32(const String& str, Error& out_error);
u32 parse_u32(const String& str, Error& out_error);

force_inline bool operator==(const String& sa, const String& sb);
force_inline bool operator==(const String& str, const char* cstr);
force_inline bool operator==(const char* cstr, const String& str);
force_inline bool operator!=(const String& sa, const String& sb);
force_inline bool operator!=(const String& str, const char* cstr);
force_inline bool operator!=(const char* cstr, const String& str);

template <>
usize hash(const String& v);

// TODO(szulf): maybe implement format myself in the future?
#define fmt(buf, buf_size, format, ...) snprintf(buf, buf_size, format, __VA_ARGS__)

#endif
