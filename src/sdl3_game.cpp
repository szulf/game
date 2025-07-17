#include "game.cpp"

#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <math.h>

#include "sdl3_game.h"

i32 main()
{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

#ifdef GAME_INTERNAL
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  auto *window = SDL_CreateWindow("game", 640, 480, SDL_WINDOW_RESIZABLE);
  if (window == nullptr)
  {
    fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
    return 1;
  }

  sdl3::AudioBuffer audio_buffer = {};
  audio_buffer.spec.format   = SDL_AUDIO_S16;
  audio_buffer.spec.channels = 2;
  audio_buffer.spec.freq     = 48000;

  auto *audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                 &audio_buffer.spec, nullptr, nullptr);
  if (!audio_stream)
  {
    fprintf(stderr, "Error opening audio stream: %s\n", SDL_GetError());
    return 1;
  }

  // TODO(szulf): do i really want just enough audio for one tick?
  audio_buffer.sample_count = (u32)
    ((audio_buffer.spec.freq * audio_buffer.spec.channels) / TPS);
  audio_buffer.size         = audio_buffer.sample_count * sizeof(i16);
  audio_buffer.memory       = (i16*) SDL_malloc(audio_buffer.size);

  SDL_Event e;
  bool running = true;

  i8  update_tick = 0;
  i8  last_tick   = 0;
  u32 last_second = 0;
  u64 starter_ms  = SDL_GetTicks();

  SDL_ResumeAudioStreamDevice(audio_stream);

  while (running)
  {
    u64 ms;
    {
      ms = SDL_GetTicks() - starter_ms;
      u32 current_second_ms = ms % 1000;
      u32 current_second = (u32) (ms / 1000);

      if (current_second != last_second)
      {
        update_tick = 0;
      }

      update_tick = (i8) (current_second_ms / MSPT);

      last_second = current_second;
    }

    {
      while (SDL_PollEvent(&e))
      {
        switch (e.type)
        {
          case SDL_EVENT_QUIT:
          {
            running = false;
          } break;
        }
      }
    }

    {
      i8 safe_update_tick = update_tick;
      if (last_tick > update_tick)
      {
        safe_update_tick += 20;
      }

      for (i8 tick = last_tick; tick < safe_update_tick; ++tick)
      {
        // NOTE(szulf): to really get the current tick you have to do tick % 20

        game::SoundBuffer sound_buffer = {};
        sound_buffer.memory = audio_buffer.memory;
        sound_buffer.size = audio_buffer.size;
        sound_buffer.sample_count = audio_buffer.sample_count;
        sound_buffer.samples_per_second = (u32) audio_buffer.spec.freq;

        game::get_sound(&sound_buffer);
        if (!SDL_PutAudioStreamData(audio_stream, sound_buffer.memory, (i32) sound_buffer.size))
        {
          fprintf(stderr, "Error putting audio stream data: %s\n", SDL_GetError());
          return 1;
        }
      }
    }

    {
      u64 new_ms = SDL_GetTicks() - starter_ms;
      u64 ms_in_frame = new_ms - ms;

      if (ms_in_frame < MSPF)
      {
        SDL_Delay((u32) (MSPF - ms_in_frame));
      }

      last_tick = update_tick;
    }
  }

  return 0;
}
