#ifndef BASE_H
#define BASE_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef ptrdiff_t isize;

typedef float f32;
typedef double f64;

#define U8_MIN 0u
#define U8_MAX 0xffu
#define U16_MIN 0u
#define U16_MAX 0xffffu
#define U32_MIN 0u
#define U32_MAX 0xffffffffu
#define U64_MIN 0ull
#define U64_MAX 0xffffffffffffffffull

#define I8_MIN (-0x7f - 1)
#define I8_MAX 0x7f
#define I16_MIN (-0x7fff - 1)
#define I16_MAX 0x7fff
#define I32_MIN (-0x7fffffff - 1)
#define I32_MAX 0x7fffffff
#define I64_MIN (-0x7fffffffffffffffll - 1)
#define I64_MAX 0x7fffffffffffffffll

#define F32_MIN 1.17549435e-38f
#define F32_MAX 3.40282347e+38f
#define F64_MIN 2.2250738585072014e-308
#define F64_MAX 1.7976931348623157e+308

// TODO(szulf): maybe implement printf myself in the future
#define print(...) printf(__VA_ARGS__)

#define unused(x) ((void) (sizeof(x)))

#define array_size(arr) (sizeof(arr) / sizeof(*arr))

#ifdef COMPILER_CLANG
#  define force_inline __attribute((always_inline))
#else
#  error Unknown compiler, Supported compilers: clang
#endif

#ifdef COMPILER_CLANG
#  define breakpoint() __builtin_trap()
#else
#  error Unknown compiler, Supported compilers: clang
#endif

#ifdef COMPILER_CLANG
#  define dll_export extern "C" __attribute((visibility("default")))
#else
#  error Unknown compiler, Supported compilers: clang
#endif

#ifdef ASSERTIONS
#  define ASSERT(expr, ...)                                                                        \
    do                                                                                             \
    {                                                                                              \
      if (!(expr))                                                                                 \
      {                                                                                            \
        print("assertion failed on expression: %s\nwith message: ", #expr);                        \
        print(__VA_ARGS__);                                                                        \
        print("\n");                                                                               \
        breakpoint();                                                                              \
      }                                                                                            \
    }                                                                                              \
    while (false)
#else
#  define ASSERT(expr, msg)
#endif

template <typename T>
struct RemoveReference
{
  typedef T Type;
};
template <typename T>
struct RemoveReference<T&>
{
  typedef T Type;
};
template <typename T>
struct RemoveReference<T&&>
{
  typedef T Type;
};
template <typename T>
inline T&& forward(typename RemoveReference<T>::Type& t)
{
  return static_cast<T&&>(t);
}
template <typename T>
inline T&& forward(typename RemoveReference<T>::Type&& t)
{
  return static_cast<T&&>(t);
}

template <typename F>
struct DeferImpl
{
  F f;
  DeferImpl(F fn) : f{forward<F>(fn)} {}
  DeferImpl(const DeferImpl& other) = default;
  ~DeferImpl()
  {
    f();
  }
};
template <typename F>
DeferImpl<F> make_defer(F&& f)
{
  return DeferImpl<F>(forward<F>(f));
}
#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x) DEFER_2(x, __COUNTER__)
#define defer(code)                                                                                \
  auto DEFER_3(_defer) = make_defer(                                                               \
    [&]()                                                                                          \
    {                                                                                              \
      code;                                                                                        \
    }                                                                                              \
  )

enum GlobalError
{
  SUCCESS = 0,
  GLOBAL_ERROR_NOT_FOUND,
  GLOBAL_ERROR_INVALID_DATA,
  GLOBAL_ERROR_OUT_OF_MEMORY,
  GLOBAL_ERROR_COUNT,
};
typedef u32 Error;

#ifdef MODE_DEBUG
#  include <unistd.h>
#  define ERROR_ASSERT(expr, err_var, err_val, ret_val)                                            \
    ERROR_ASSERT_1(expr, err_var, err_val, ret_val, __LINE__, __FILE__)
#  define ERROR_ASSERT_1(expr, err_var, err_val, ret_val, line, file)                              \
    ERROR_ASSERT_2(expr, err_var, err_val, ret_val, line, file)
#  define ERROR_ASSERT_2(expr, err_var, err_val, ret_val, line, file)                              \
    do                                                                                             \
    {                                                                                              \
      if (!(expr))                                                                                 \
      {                                                                                            \
        print("%s %s %s\n", #expr, #line, #file);                                                  \
        (err_var) = (err_val);                                                                     \
        return ret_val;                                                                            \
      }                                                                                            \
    }                                                                                              \
    while (0)
#else
#  define ERROR_ASSERT(expr, err_var, err_val, ret_val)                                            \
    do                                                                                             \
    {                                                                                              \
      if (!(expr))                                                                                 \
      {                                                                                            \
        (err_var) = (err_val);                                                                     \
        return ret_val;                                                                            \
      }                                                                                            \
    }                                                                                              \
    while (0)
#endif

template <typename T>
usize hash(const T& value);

enum Key
{
  KEY_W = 1,
  KEY_S,
  KEY_A,
  KEY_D,
  KEY_E,
  KEY_F1,
  KEY_F2,
  KEY_SPACE,
  KEY_LSHIFT,
};

#endif
