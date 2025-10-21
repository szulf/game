#ifndef STRING_H
#define STRING_H

// NOTE(szulf): these are immutable
// NOTE(szulf): these are not null terminated
struct String
{
  usize len;
  const char* data;

  static String make(const char* cstr);
  static String make(const char* cstr, usize len);

  usize count_chars(char c) const;
  usize find_char(char c, usize start_idx = 0) const;
  String prepend(const char* cstr, mem::Arena& arena, Error* err) const;
  Array<String> split(char c, mem::Arena& arena, Error* err) const;
  template <typename T>
  T parse(Error* err) const;

  const char& operator[](usize idx) const;

  const char* begin() const;
  const char* end() const;
};

static usize cstr_len(const char* c_str);

template <typename... Args>
static String format(const mem::Arena& arena, const char* fmt_, const Args&... args);

#endif

