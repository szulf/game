#ifndef STRING_H
#define STRING_H

bool is_whitespace(char c);
usize cstr_len(const char* cstr);

// NOTE: immutable and not null-terminated
struct String
{
  const char* data;
  usize size;

  static String make(const char* cstr);
  static String make(const char* cstr, usize len);

  inline char operator[](usize idx) const;

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

inline bool operator==(const String& sa, const String& sb);
inline bool operator==(const String& str, const char* cstr);
inline bool operator==(const char* cstr, const String& str);
inline bool operator!=(const String& sa, const String& sb);
inline bool operator!=(const String& str, const char* cstr);
inline bool operator!=(const char* cstr, const String& str);

template <>
usize hash(const String& v);

// TODO: maybe implement format myself in the future?
#define fmt(buf, buf_size, format, ...) snprintf(buf, buf_size, format, __VA_ARGS__)

#endif
