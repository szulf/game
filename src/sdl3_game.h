#ifndef SDL_GAME_H
#define SDL_GAME_H

typedef struct SDL3AudioBuffer
{
  SDL_AudioSpec spec;
  s16* memory;
  u32 size; // NOTE(szulf): needed to be a u32 because of SDL_LoadWAV
            //              (another reason to write my own loader)(would want it to be a usize)
  u32 sample_count;
  u32 sample_index;
} SDL3AudioBuffer;

#endif
