#include "string.hpp"
#include "math.hpp"

namespace btl {

usize cstrLen(const char* cstr) {
  const char* s;
  for (s = cstr; *s; ++s) {
  }
  return static_cast<usize>(s - cstr);
}

String String::make(const char* cstr) {
  String s;
  s.data = cstr;
  s.size = cstrLen(cstr);
  return s;
}

String String::make(const char* cstr, usize len) {
  String s;
  s.data = cstr;
  s.size = len;
  return s;
}

usize String::count_chars(char c) const {
  usize count = 0;
  for (auto ch : *this) {
    if (ch == c) {
      ++count;
    }
  }
  return count;
}

usize String::find_char(char c, usize start_idx) const {
  ASSERT(start_idx < size, "start_idx is out of bounds");
  for (usize i = start_idx; i < size; ++i) {
    if (data[i] == c) {
      return i;
    }
  }
  return static_cast<usize>(-1);
}

String String::append(const char* cstr, Allocator& allocator) const {
  usize cstr_len = cstrLen(cstr);
  char* out = static_cast<char*>(allocator.alloc(size + cstr_len));
  mem::copy(out, data, size);
  mem::copy(out + size, cstr, cstr_len);
  return String::make(out, size + cstr_len);
}

String String::append(const String& str, Allocator& allocator) const {
  char* out = static_cast<char*>(allocator.alloc(size + str.size));
  mem::copy(out, data, size);
  mem::copy(out + size, str.data, str.size);
  return String::make(out, size + str.size);
}

const char* String::c_string(Allocator& allocator) const {
  char* out = static_cast<char*>(allocator.alloc(size + 1));
  mem::copy(out, data, size);
  out[size] = 0;
  return out;
}

// TODO(szulf): this whole implementation is kinda whacky but idc for now
List<String> String::split(char c, Allocator& arena) const {
  usize splits_count = count_chars(c) + 1;
  auto splits = List<String>::make(splits_count, arena);

  usize start_idx = 0;
  for (usize found_idx = find_char(c, start_idx); found_idx != static_cast<usize>(-1);
       found_idx = find_char(c, start_idx)) {
    auto s = String::make(data + start_idx, found_idx - start_idx);
    start_idx = found_idx + 1;
    if (s.size != 0) {
      splits.push(s);
    }
    if (start_idx >= size) {
      break;
    }
  }

  if (size - start_idx > 0) {
    auto s = String::make(data + start_idx, size - start_idx);
    splits.push(s);
  }

  return splits;
}

String String::copy(Allocator& allocator) const {
  return make(c_string(allocator), size);
}

template <>
Result<f32, ParseError> String::parse<f32>() const {
  const char* s = data;

  f32 sign = 1.0f;
  if (*s == '-') {
    sign = -1.0f;
    ++s;
  } else if (*s == '+') {
    ++s;
  }

  bool is_fraction = false;
  f32 val = 0.0f;
  f32 frac = 0.0f;
  f32 divisor = 1.0f;
  while (s < data + size) {
    if (*s == '.') {
      if (is_fraction) {
        return err<f32>(ParseError::InvalidInput);
      }
      is_fraction = true;
      ++s;
    }
    if (*s < '0' && *s > '9') {
      return err<f32>(ParseError::InvalidInput);
    }
    if (!is_fraction) {
      val = val * 10.0f + (*s - '0');
    } else {
      divisor *= 10.0f;
      frac = frac + (*s - '0') / divisor;
    }
    ++s;
  }

  return ok<ParseError>(sign * (val + frac));
}

template <>
Result<u32, ParseError> String::parse<u32>() const {
  const char* s = data;

  u32 val = 0;
  while (s < data + size) {
    if (*s < '0' && *s > '9') {
      return err<u32>(ParseError::InvalidInput);
    }
    val = val * 10u + static_cast<u32>(*s - '0');
    ++s;
  }

  return ok<ParseError>(val);
}

String to_string(const String& v) {
  return v;
}

}

btl::usize hash(const btl::String& v) {
  btl::usize hash = btl::constants<btl::u64>::fnv_offset;
  for (btl::usize i = 0; i < v.size; ++i) {
    hash ^= reinterpret_cast<const btl::u8*>(v.data)[i];
    hash *= btl::constants<btl::u64>::fnv_prime;
  }
  return hash;
}
