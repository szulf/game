#include "game.h"

consteval u64
kilobytes(u64 n)
{
  return n * 1024ll;
}

consteval u64
megabytes(u64 n)
{
  return kilobytes(n) * 1024ll;
}

consteval u64
gigabytes(u64 n)
{
  return megabytes(n) * 1024ll;
}

namespace game
{

static void
setup_default_keybinds()
{
  g_keybind_map[static_cast<usize>(Key::LMB)] = Action::ChangeScene;
  g_keybind_map[static_cast<usize>(Key::Space)] = Action::Move;
}

// TODO(szulf): delete this later
static Scene
setup_simple_scene(const char* obj_path)
{
  Error error = Error::Success;

  Mesh mesh = Mesh::from_obj(obj_path, &error);
  ASSERT(error == Error::Success, "couldnt load sphere mesh");

  std::pmr::vector<Mesh> meshes{};
  meshes.push_back(mesh);

  Model model = {};
  model.meshes = meshes;
  model.model = Mat4(1.0f);
  std::pmr::vector<Drawable> drawables{};
  drawables.push_back({model, Shader::Default});

  Scene scene = {};
  scene.drawables = drawables;
  scene.view = Mat4(1.0f);
  scene.proj = Mat4(1.0f);

  return scene;
}

static void
setup(State& state)
{
  Error error = Error::Success;

  setup_renderer();
  setup_default_keybinds();

  setup_shaders(&error);
  ASSERT(error == Error::Success, "couldnt initialize shaders");

  Scene sphere_scene = setup_simple_scene("assets/sphere.obj");
  Scene cube_scene = setup_simple_scene("assets/cube.obj");
  Scene cone_scene = setup_simple_scene("assets/cone.obj");

  state.current_scene_idx = 0;
  state.scenes.reserve(3);
  state.scenes.emplace_back(std::move(sphere_scene));
  state.scenes.emplace_back(std::move(cube_scene));
  state.scenes.emplace_back(std::move(cone_scene));
}

static void
update(State& state, Input& input)
{
  static f32 degree = 0.0f;
  static f32 move = -3.0f;

  for (usize i = 0; i < input.input_events.size(); ++i)
  {
    switch (g_keybind_map[static_cast<usize>(input.input_events[i].key)])
    {
      case Action::ChangeScene:
      {
        state.current_scene_idx += 1;
        state.current_scene_idx %= state.scenes.size();
      } break;
      case Action::Move:
      {
        move -= 1.0f;
      } break;
    };
  }
  input.input_events.clear();

  auto& scene = state.scenes[state.current_scene_idx];

  Vec3 translation_vec = {0.0f, 0.0f, move};
  scene.view.translate(&translation_vec);

  auto dimensions = platform::get_window_dimensions();
  scene.proj = Mat4::perspective(radians(45.0f), (f32) dimensions.width / (f32) dimensions.height,
                                  0.1f, 100.0f);

  Vec3 rotate_vec = {1.0f, 1.0f, 0.0f};
  scene.drawables[0].model.rotate(degree, &rotate_vec);

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
get_sound(SoundBuffer* sound_buffer)
{
  static u32 sample_index = 0;

  for (u32 i = 0; i < sound_buffer->sample_count; i += 2)
  {
    f32 t = (f32) sample_index / (f32) sound_buffer->samples_per_second;
    f32 frequency = 440.0f;
    f32 amplitude = 0.25f;
    i16 sine_value = (i16) (std::sin(2.0f * PI32 * t * frequency) * I16_MAX * amplitude);
    UNUSED(sine_value);

    ++sample_index;

    i16* left  = sound_buffer->memory + i;
    i16* right = sound_buffer->memory + i + 1;

    // *left  = sine_value;
    // *right = sine_value;
    *left  = 0;
    *right = 0;
  }
}

}
