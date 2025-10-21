#include "game.h"

template <typename... Args> static void
log_(const char* file, usize line, const char* func, const char* fmt, const Args&... args)
{
  static u8 depth = 0;
  ++depth;
  if (depth > 3) return;

  u8 buffer[4096] = {};

  mem::Arena arena = {};
  arena.buffer = buffer;
  arena.buffer_size = 4096;

  String formatted = format(arena, fmt, args...);
  String final = format(arena, "[({}) {}:{}] {}", func, file, line, formatted);
  os::print(final.data);
  --depth;
}

namespace game
{

// TODO(szulf): delete this later
static Scene
setup_simple_scene(const char* obj_path, mem::Arena& perm_arena, mem::Arena& temp_arena)
{
  Error error = Error::SUCCESS;

  Model model = obj::parse(obj_path, temp_arena, perm_arena, &error);
  ASSERT(error == Error::SUCCESS, "couldnt load obj model");

  Array<Renderable> renderables = Array<Renderable>::make(1, perm_arena, &error);
  ASSERT(error == Error::SUCCESS, "couldnt init renderables array");
  renderables.push({model, Shader::DEFAULT});

  Scene scene = {};
  scene.renderables = renderables;
  scene.view = Mat4::make(1.0f);
  scene.proj = Mat4::make(1.0f);

  return scene;
}

static void
setup(State& state, mem::Arena& temp_arena, mem::Arena& perm_arena)
{
  Error error = Error::SUCCESS;

  // TODO(szulf): do i want all these setup functions?
  setup_renderer();
  setup_shaders(temp_arena, &error);
  ASSERT(error == Error::SUCCESS, "couldnt initialize shaders");
  assets::setup(perm_arena, &error);
  ASSERT(error == Error::SUCCESS, "couldnt initialize assets");
  setup_default_keybinds();

  Scene backpack_scene = setup_simple_scene("assets/backpack.obj", perm_arena, temp_arena);
  Scene sphere_scene = setup_simple_scene("assets/sphere.obj", perm_arena, temp_arena);
  Scene cube_scene = setup_simple_scene("assets/cube.obj", perm_arena, temp_arena);
  Scene cone_scene = setup_simple_scene("assets/cone.obj", perm_arena, temp_arena);

  state.current_scene_idx = 0;
  state.scenes = Array<Scene>::make(4, perm_arena, &error);
  ASSERT(error == Error::SUCCESS, "couldnt init scenes array");
  state.scenes.push(backpack_scene);
  state.scenes.push(sphere_scene);
  state.scenes.push(cube_scene);
  state.scenes.push(cone_scene);
}

static void
update(State& state, Input& input)
{
  static f32 degree = 0.0f;
  static f32 move = -3.0f;

  for (const auto& input_event : input.input_events)
  {
    switch (keybind_map[(usize) input_event.key])
    {
      case Action::CHANGE_SCENE:
      {
        state.current_scene_idx += 1;
        state.current_scene_idx %= state.scenes.len;
      } break;
      case Action::MOVE:
      {
        move -= 1.0f;
      } break;
    };
  }
  input.input_events.len = 0;

  Scene& scene = state.scenes[state.current_scene_idx];

  Vec3 translation_vec = {0.0f, 0.0f, move};
  scene.view.translate(translation_vec);

  auto dimensions = os::get_window_dimensions();
  scene.proj = Mat4::perspective(radians(45.0f), (f32) dimensions.width / (f32) dimensions.height,
                                  0.1f, 100.0f);

  Vec3 rotate_vec = {1.0f, 1.0f, 0.0f};
  scene.renderables[0].model.rotate(degree, rotate_vec);

  degree += 1.0f;
}

static void
render(State& state)
{
  Scene& curr_scene = state.scenes[state.current_scene_idx];

  clear_screen();

  curr_scene.draw();
}

static void
get_sound(SoundBuffer& sound_buffer)
{
  static u32 sample_index = 0;

  for (u32 i = 0; i < sound_buffer.sample_count; i += 2)
  {
    f32 t = (f32) sample_index / (f32) sound_buffer.samples_per_second;
    f32 frequency = 440.0f;
    f32 amplitude = 0.25f;
    i16 sine_value = (i16) (sin(2.0f * PI32 * t * frequency) * I16_MAX * amplitude);
    UNUSED(sine_value);

    ++sample_index;

    i16* left  = sound_buffer.memory + i;
    i16* right = sound_buffer.memory + i + 1;

    // *left  = sine_value;
    // *right = sine_value;
    *left  = 0;
    *right = 0;
  }
}

static void
setup_default_keybinds()
{
  keybind_map[(usize) Key::LMB] = Action::CHANGE_SCENE;
  keybind_map[(usize) Key::SPACE] = Action::MOVE;
}

}

