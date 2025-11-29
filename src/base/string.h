#ifndef STRING_H
#define STRING_H

usize cstr_len(const char* cstr);

// NOTE(szulf): immutable and not null-terminated
struct String
{
  const char* data;
  usize size;

  force_inline char operator[](usize idx) const;
};

String string_make(const char* cstr);
String string_make_len(const char* cstr, usize len);

usize string_count_chars(const String& str, char c);
usize string_find_char(const String& str, char c, usize start_idx);
String string_append_cstr(const String& str, const char* cstr, Allocator& allocator);
String string_append_str(const String& s1, const String& s2, Allocator& allocator);
const char* string_to_cstr(const String& str, Allocator& allocator);
Array<String> string_split(const String& str, char c, Allocator& allocator);
String string_copy(const String& str, Allocator& allocator);
f32 string_parse_f32(const String& str, Error& out_error);
u32 string_parse_u32(const String& str, Error& out_error);

force_inline bool operator==(const String& sa, const String& sb);
force_inline bool operator==(const String& str, const char* cstr);
force_inline bool operator==(const char* cstr, const String& str);
force_inline bool operator!=(const String& sa, const String& sb);
force_inline bool operator!=(const String& str, const char* cstr);
force_inline bool operator!=(const char* cstr, const String& str);

template <>
usize hash(const String& v);

#endif
