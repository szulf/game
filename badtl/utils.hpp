#ifndef BADTL_UTILS_HPP
#define BADTL_UTILS_HPP

namespace btl {

#ifdef BADTL_COMPILER_CLANG
#  define btl_always_inline __attribute((always_inline))
#else
#  error Unknown compiler
#endif

#ifdef BADTL_COMPILER_CLANG
#  define btl_breakpoint() __builtin_trap()
#else
#  error Unknown compiler
#endif

template <typename F>
struct DeferImpl {
  F f;
  DeferImpl(F fn) : f{fn} {}
  DeferImpl(const DeferImpl& other) = default;
  ~DeferImpl() {
    f();
  }
};

template <typename F>
DeferImpl<F> make_defer(F f) {
  return DeferImpl<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x) DEFER_2(x, __COUNTER__)
#define defer(code)                                                                                                    \
  auto DEFER_3(_defer) = btl::make_defer([&]() {                                                                       \
    code;                                                                                                              \
  })

}

#ifdef GAME_DEBUG
#  include "print.hpp"
#  define ASSERT(expr, msg)                                                                                            \
    do {                                                                                                               \
      if (!(expr)) {                                                                                                   \
        btl::print("assertion failed on expression: {}\nwith message: {}\n", #expr, (msg));                            \
        btl_breakpoint();                                                                                              \
      }                                                                                                                \
    } while (false)
#else
#  define ASSERT(expr, msg)
#endif

#endif
