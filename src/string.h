#ifndef STRING_H
#define STRING_H

// NOTE(szulf): these are immutable
// NOTE(szulf): these are not null terminated
struct String
{
  usize len;
  const char* data;

  const char& operator[](usize idx) const;
};

static String string_make_cstr(const char* cstr);
static String string_make_cstr_len(const char* cstr, usize len);

static usize string_count_chars(String* str, char c);
static usize string_find_char(String* str, char c, usize start_idx);

static String string_prepend(String* str, const char* cstr, Arena* arena, Error* err);
template <typename... Args>
static String string_format(Arena* arena, const char* fmt_, const Args&... args);

static Array<String> string_split(String* str, char c, Arena* arena, Error* err);

static f32 string_parse_f32(String* str, Error* err);
static u32 string_parse_u32(String* str, Error* err);

static usize cstr_len(const char* c_str);

#endif

