struct ivec2 {
  i32 x{};
  i32 y{};
};

ivec2 operator+(const ivec2& a, const ivec2& b) {
  return {a.x + b.x, a.y + b.y};
}

ivec2 operator-(const ivec2& a, const ivec2& b) {
  return {a.x - b.x, a.y - b.y};
}

// TODO: should i keep this?
ivec2 operator*(const ivec2& a, f32 scalar) {
  return {i32(f32(a.x * scalar)), i32(f32(a.y * scalar))};
}

ivec2 operator*(const ivec2& a, i32 scalar) {
  return {a.x * scalar, a.y * scalar};
}

ivec2 operator*(const ivec2& a, const ivec2& b) {
  return {a.x * b.x, a.y * b.y};
}

ivec2 operator/(const ivec2& a, i32 b) {
  return {a.x / b, a.y / b};
}

ivec2 operator/(const ivec2& a, const ivec2& b) {
  return {a.x / b.x, a.y / b.y};
}

ivec2& operator+=(ivec2& a, const ivec2& b) {
  a.x += b.x;
  a.y += b.y;
  return a;
}

bool operator==(const ivec2& a, const ivec2& b) {
  return a.x == b.x && a.y == b.y;
}

i32 length2(const ivec2& v) {
  return (v.x * v.x) + (v.y * v.y);
}
