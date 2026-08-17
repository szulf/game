#pragma once

#include <cstdint>
#include <print>
#include <limits>
#include <type_traits>

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

static constexpr u16 U16_MAX = std::numeric_limits<u16>::max();
static constexpr f32 F32_MAX = std::numeric_limits<f32>::max();

#define ASSERT_NO_MSG(expr)                                                                        \
  do {                                                                                             \
    if (!(expr)) {                                                                                 \
      std::println("Assertion (%s:%d) failed on expression: '%s'\n", __FILE__, __LINE__, #expr);   \
      std::abort();                                                                                \
    }                                                                                              \
  } while (false)

#define ASSERT(expr, ...)                                                                          \
  do {                                                                                             \
    if (!(expr)) {                                                                                 \
      std::println(                                                                                \
        "Assertion (%s:%d) failed on expression: '%s' with message:\n",                            \
        __FILE__,                                                                                  \
        __LINE__,                                                                                  \
        #expr                                                                                      \
      );                                                                                           \
      std::println(__VA_ARGS__);                                                                   \
      std::println("\n");                                                                          \
      std::abort();                                                                                \
    }                                                                                              \
  } while (false)

// NOTE: for std::visit
template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename T, typename... Ts>
inline constexpr bool is_any_of = (std::is_same_v<T, Ts> || ...);
