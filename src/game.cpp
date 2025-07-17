#include "game.h"

#include <math.h>
#include <stdio.h>

namespace game {

static void get_sound(SoundBuffer *sound_buffer)
{
  static u32 sample_index = 0;

  for (u32 i = 0; i < sound_buffer->sample_count; i += 2)
  {
    f32 t = (f32) sample_index / (f32) sound_buffer->samples_per_second;
    f32 frequency = 440.0f;
    f32 amplitude = 0.25f;
    // TODO(szulf): maybe implement my own sinf?
    i16 sine_value = (i16) (sinf(2.0f * PI32 * t * frequency) * I16_MAX * amplitude);

    ++sample_index;

    i16 *left  = sound_buffer->memory + i;
    i16 *right = sound_buffer->memory + i + 1;

    *left  = sine_value;
    *right = sine_value;
  }
}

}
