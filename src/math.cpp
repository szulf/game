struct vec2 {
  f32 x{};
  f32 y{};
};

vec2 operator+(const vec2& a, const vec2& b) {
  return {a.x + b.x, a.y + b.y};
}

vec2 operator-(const vec2& a, const vec2& b) {
  return {a.x - b.x, a.y - b.y};
}

vec2 operator*(const vec2& a, f32 scalar) {
  return {a.x * scalar, a.y * scalar};
}

vec2 operator*(const vec2& a, const vec2& b) {
  return {a.x * b.x, a.y * b.y};
}

vec2 operator/(const vec2& a, i32 b) {
  return {a.x / b, a.y / b};
}

vec2 operator/(const vec2& a, const vec2& b) {
  return {a.x / b.x, a.y / b.y};
}

vec2& operator+=(vec2& a, const vec2& b) {
  a.x += b.x;
  a.y += b.y;
  return a;
}

bool operator==(const vec2& a, const vec2& b) {
  return a.x == b.x && a.y == b.y;
}

f32 length(const vec2& v) {
  return std::hypot(v.x, v.y);
}

f32 length2(const vec2& v) {
  return (v.x * v.x) + (v.y * v.y);
}
