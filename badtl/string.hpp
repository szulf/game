#ifndef BADTL_STRING_HPP
#define BADTL_STRING_HPP

#include "utils.hpp"
#include "allocator.hpp"
#include "list.hpp"
#include "types.hpp"
#include "result.hpp"

namespace btl {

usize cstrLen(const char* cstr);

enum class ParseError {
  InvalidInput,
};

// NOTE(szulf): immutable and not null-terminated
struct String {
  const char* data;
  usize size;

  static String make(const char* cstr);
  static String make(const char* cstr, usize len);
  usize count_chars(char c) const;
  usize find_char(char c, usize start_idx = 0) const;
  String append(const char* cstr, Allocator& allocator) const;
  String append(const String& str, Allocator& allocator) const;
  const char* c_string(Allocator& allocator) const;
  List<String> split(char c, Allocator& allocator) const;
  String copy(Allocator& allocator) const;

  template <typename T>
  Result<T, ParseError> parse() const;

  inline char operator[](usize idx) const;
  inline bool operator==(const String& other) const;
  inline bool operator==(const char* cstr) const;
  inline bool operator!=(const String& other) const;
  inline bool operator!=(const char* cstr) const;

  inline const char* begin() const;
  inline const char* end() const;
};

String to_string(const String& v);

}

btl::usize hash(const btl::String& v);

namespace btl {

inline char String::operator[](usize idx) const {
  ASSERT(idx < size, "string index out of bounds");
  return data[idx];
}

inline bool String::operator==(const String& other) const {
  return size == other.size && btl::mem::eql(data, other.data, size);
}

inline bool String::operator==(const char* cstr) const {
  return size == cstrLen(cstr) && btl::mem::eql(data, cstr, size);
}

inline bool String::operator!=(const String& other) const {
  return !(*this == other);
}

inline bool String::operator!=(const char* cstr) const {
  return !(*this == cstr);
}

inline const char* String::begin() const {
  return data;
}

inline const char* String::end() const {
  return data + size;
}

}

#endif
