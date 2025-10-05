#ifndef STRING_H
#define STRING_H

// TODO(szulf): should these be null terminated?
typedef struct String
{
  usize cap;
  usize len;
  char* data;
} String;

static String string_make_cstr(Arena* arena, const char* cstr, Error* err);
static String string_make_cstr_len(Arena* arena, const char* cstr, usize len, Error* err);
static String string_make_cap(Arena* arena, usize cap, Error* err);

static usize string_count_chars(String* str, char c);
static usize string_find_char(String* str, char c, usize start_idx, Error* err);

typedef struct StringArray
{
  usize cap;
  usize len;
  String* items;
} StringArray;

static StringArray string_split(String* str, Arena* arena, char c, Error* err);

static f32 string_parse_f32(String* str, Error* err);
static u32 string_parse_u32(String* str, Error* err);

static usize cstr_len(const char* c_str);

#endif

