#ifndef _SDL_GAME_H
#define _SDL_GAME_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;

typedef i32 bool32;
typedef size_t usize;

typedef float f32;
typedef double f64;

#define PI32 3.141592653f

#define I16_MAX  32767
#define I16_MIN -32768

struct SDL3AudioBuffer
{
  SDL_AudioSpec spec;
  void *buffer;
  u32 size; // NOTE(szulf): needed to be a u32 because of SDL_LoadWAV
            //              (another reason to write my own loader)(would want it to be a usize)
  u32 sample_count;
};

#endif
