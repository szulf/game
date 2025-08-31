#ifndef STRING_H
#define STRING_H

// TODO(szulf): should these be null terminated?
typedef struct String
{
  usize cap;
  usize len;
  char* data;
} String;

static Error string_init_cstr(String* str, Arena* arena, const char* cstr);
static Error string_init_cstr_len(String* str, Arena* arena, const char* cstr, usize len);
static Error string_init_cap(String* str, Arena* arena, usize cap);

static usize string_count_chars(String* str, char c);
static Error string_find_char(usize* found_idx, String* str, char c, usize start_idx);

typedef struct StringArray
{
  usize cap;
  usize len;
  String* items;
} StringArray;

static Error string_split(StringArray* splits, String* str, Arena* arena, char c);

static Error string_parse_f32(f32* out, String* str);
static Error string_parse_u32(u32* out, String* str);

static usize cstr_len(const char* c_str);

#endif
