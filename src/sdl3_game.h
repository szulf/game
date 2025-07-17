#ifndef SDL_GAME_H
#define SDL_GAME_H

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

namespace sdl3
{

struct AudioBuffer
{
  SDL_AudioSpec spec;
  i16 *memory;
  u32 size; // NOTE(szulf): needed to be a u32 because of SDL_LoadWAV
            //              (another reason to write my own loader)(would want it to be a usize)
  u32 sample_count;
  u32 sample_index;
};

}

#endif
