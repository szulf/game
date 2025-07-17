#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t   u8;
typedef int8_t    i8;
typedef uint16_t u16;
typedef int16_t  i16;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint64_t u64;
typedef int64_t  i64;

typedef i32    bool32;
typedef size_t usize;

typedef float  f32;
typedef double f64;

#define PI32 3.141592653f

#define I16_MAX  32767
#define I16_MIN -32768

#define FPS  10
#define MSPF (1000 / FPS)

#define TPS  20
#define MSPT (1000 / TPS)

// TODO(szulf): maybe create like a math or utils file for these?
template <typename T>
inline T min(T a, T b)
{
  if (a > b)
  {
    return b;
  }

  return a;
}

template <typename T>
inline T max(T a, T b)
{
  if (a > b)
  {
    return a;
  }

  return b;
}

namespace game
{

struct SoundBuffer
{
  i16 *memory;
  usize size;
  u32 sample_count;
  u32 samples_per_second;
};

static void get_sound(SoundBuffer *sound_buffer);

};

#endif
