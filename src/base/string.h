#ifndef STRING_H
#define STRING_H

usize cstr_len(const char* cstr);

// NOTE(szulf): immutable and not null-terminated
struct String
{
  const char* data;
  usize size;
};

String string_make(const char* cstr);
String string_make_len(const char* cstr, usize len);

usize string_count_chars(const String* str, char c);
usize string_find_char(const String* str, char c, usize start_idx);
String string_append_cstr(const String* str, const char* cstr, Allocator* allocator);
String string_append_str(const String* s1, const String* s2, Allocator* allocator);
const char* string_to_cstr(const String* str, Allocator* allocator);
Array<String> string_split(const String* str, char c, Allocator* allocator);
String string_copy(const String* str, Allocator* allocator);
f32 string_parse_f32(const String* str, Error* out_error);
u32 string_parse_u32(const String* str, Error* out_error);

force_inline char string_get(const String* str, usize idx);
template <>
force_inline b8 equal(const String* s1, const String* s2);
template <>
force_inline b8 equal(const String* s1, const char* cstr);
template <>
force_inline b8 equal(const char* cstr, const String* s1);

template <>
usize hash(const String* v);

#endif
