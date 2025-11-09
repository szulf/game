#include "game.h"

template <typename... Args>
static void log_(const char* file, usize line, const char* func, const char* fmt, const Args&...)
{
  UNUSED(file);
  UNUSED(line);
  UNUSED(func);
  UNUSED(fmt);
}

namespace game
{

Game::Game()
{
  current_scene_idx = 0;
  // scenes.push_back(Scene{{{obj::parse("assets/backpack.obj"), Shader::Default}}});
  scenes.push_back(Scene{{{obj::parse("assets/sphere.obj"), Shader::Default}}});
  // scenes.push_back(Scene{{{obj::parse("assets/cube.obj"), Shader::Default}}});
  // scenes.push_back(Scene{{{obj::parse("assets/cone.obj"), Shader::Default}}});
}

void Game::update(Input& input)
{
  static f32 degree{0.0f};
  static f32 move{-3.0f};

  for (const auto& input_event : input.input_events)
  {
    switch (keybind_map[input_event.key])
    {
      case Action::ChangeScene:
      {
        current_scene_idx += 1;
        current_scene_idx %= scenes.size();
      }
      break;
      case Action::Move:
      {
        move -= 1.0f;
      }
      break;
    };
  }
  input.input_events.clear();

  auto& scene{scenes[current_scene_idx]};

  const math::vec3 translation_vec{0.0f, 0.0f, move};
  scene.view.translate(translation_vec);

  // TODO(szulf): this only changes when the dimensions change so probably shouldn't be here
  const auto dimensions{get_window_dimensions()};
  scene.proj = math::mat4::perspective(
    math::radians(45.0f),
    static_cast<f32>(dimensions.first) / static_cast<f32>(dimensions.second),
    0.1f,
    100.0f
  );

  scene.renderables[0].model.rotate(degree, {1.0f, 1.0f, 0.0f});

  degree += 1.0f;
}

void Game::update_frame()
{
  static f32 interp = 0.0f;
  auto& scene{scenes[current_scene_idx]};
  for (auto& renderable : scene.renderables)
  {
    std::println("visible: {}", renderable.model.visible_rotation);
    std::println("actual: {}", renderable.model.rotation);
    std::println(
      "slerp: {}",
      math::quat::slerp(renderable.model.visible_rotation, renderable.model.rotation, 0.5f)
    );

    renderable.model.visible_rotation =
      math::quat::slerp(renderable.model.visible_rotation, renderable.model.rotation, interp);
    renderable.model.mat = math::mat4{renderable.model.rotation, {0.0f}, {1.0f}};
    interp += 1.0f / FPS;
    f32 dummy;
    interp = std::modf(interp, &dummy);
  }
}

void Game::render()
{
  auto& scene{scenes[current_scene_idx]};

  renderer.clear_screen();
  renderer.render(scene);
}

void Game::get_sound(SoundBuffer& sound_buffer)
{
  static u32 sample_index{0};

  for (u32 i{0}; i < sound_buffer.sample_count; i += 2)
  {
    f32 t{static_cast<f32>(sample_index) / static_cast<f32>(sound_buffer.samples_per_second)};
    f32 frequency{440.0f};
    f32 amplitude{0.25f};
    i16 sine_value{static_cast<i16>(sin(2.0f * PI32 * t * frequency) * I16_MAX * amplitude)};
    UNUSED(sine_value);

    ++sample_index;

    i16* left{sound_buffer.memory + i};
    i16* right{sound_buffer.memory + i + 1};

    // *left = sine_value;
    // *right = sine_value;
    *left = 0;
    *right = 0;
  }
}
}
