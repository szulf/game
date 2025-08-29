#ifndef STRING_H
#define STRING_H

// TODO(szulf): should i define a copy constructor here
// i guess shallow copies are fine since memory is managed by the arena
struct String
{
  usize cap;
  usize len;
  char* data;

  String() : len{0} {}
  String(mem::Arena& arena, const char* c_str);
  String(mem::Arena& arena, const char* c_str, usize size);
  String(mem::Arena& arena, usize size);

  usize count(char c) const;
  Result<usize> find(char c, usize start_idx = 0) const;
  Array<String> split(mem::Arena& arena, char c) const;

  template <typename T>
  Result<T> parse() const;

  const char& operator[](usize idx) const;
  bool operator==(const String& other) const;
  bool operator==(const char* other) const;
  String operator+(const String& other) const;
};

usize c_str_len(const char* c_str);

#endif
