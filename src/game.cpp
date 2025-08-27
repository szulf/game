#include "game.h"

namespace game
{

static void setup(mem::Arena& arena, State& state)
{
  setup_shaders(arena);

  Mesh meshes[] = {Mesh::from_obj(arena, "")};
  state.model = Model{Array<Mesh>{arena, meshes, 1}};
}

static void render(State& state)
{
  Renderer::clear_screen();

  state.model.rotateY(static_cast<f32>(platform::get_ms()));
  state.model.draw(Shader::DefaultShader);
}

static void get_sound(SoundBuffer& sound_buffer)
{
  static u32 sample_index = 0;

  for (u32 i = 0; i < sound_buffer.sample_count; i += 2)
  {
    f32 t = static_cast<f32>(sample_index) / static_cast<f32>(sound_buffer.samples_per_second);
    f32 frequency = 440.0f;
    f32 amplitude = 0.25f;
    i16 sine_value = static_cast<i16>(math::sin(2.0f * PI32 * t * frequency) * I16_MAX * amplitude);

    ++sample_index;

    i16* left  = sound_buffer.memory + i;
    i16* right = sound_buffer.memory + i + 1;

    *left  = sine_value;
    *right = sine_value;
  }
}

}
