#ifndef STRING_H
#define STRING_H

// NOTE(szulf): these are immutable, changing the data field is UB
// NOTE(szulf): these are null-terminated
struct String
{
  usize cap;
  usize len;
  char* data;

  static String make_cstr(const char* cstr, Arena* arena, Error* err);
  static String make_cstr_len(const char* cstr, usize len, Arena* arena, Error* err);
  static String make_capacity(usize capacity, Arena* arena, Error* err);

  usize find_char(char c, usize start_idx, Error* err) const;
  usize count_chars(char c) const;
  usize count_substrings(const char* substr) const;

  String prepend(const char* cstr, Arena* arena, Error* err) const;

  Array<String> split(char c, Arena* arena, Error* err) const;

  template <typename T>
  T parse(Error* err) const;
};

static usize cstr_len(const char* c_str);

#endif

