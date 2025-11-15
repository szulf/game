#include <print>

#include "engine.hpp"
#include "event.hpp"
#include "renderer/camera.hpp"
#include "renderer/renderer.hpp"
#include "renderer/scene.hpp"
#include "utils/templates.hpp"

namespace game {

enum class Action {
  Interact,
};

struct Game final {
  using PrevStates = utils::type_list<>;

  Game()
    : scene{
        {"assets/backpack.obj"},
        core::Camera{core::Camera::Type::Perspective, {0.0f, 0.0f, -5.0f}}
  } {
    core::KeyMap<Action>::instance().bind(core::Key::E, Action::Interact);
  }

  void render() {
    core::renderer::clearScreen();
    core::renderer::render(scene);
  }

  void update(float) {
    scene.camera.yaw += 1.0f;
    scene.camera.updateCameraVectors();
    // std::println("{}", scene.camera.yaw);
  }

  // TODO(szulf): dont really like the ifs and the switch
  void event(const core::Event& event) {
    if (const auto* window_resize{std::get_if<core::ResizeEvent>(&event)}) {
      scene.camera.viewport_width = window_resize->width;
      scene.camera.viewport_height = window_resize->height;
    }
    if (const auto* keydown{std::get_if<core::KeydownEvent>(&event)}) {
      const auto action{core::KeyMap<Action>::instance()[keydown->key]};
      switch (action) {
        case Action::Interact: {
          std::println("interacted by pressing some key");
        } break;
      }
    }
    if (const auto* mouse_move{std::get_if<core::MouseMoveEvent>(&event)}) {
      std::println("x: {} y: {}", mouse_move->x, mouse_move->y);
    }
  }

  core::Scene scene;
};

}

std::int32_t main() {
  core::AppSpec spec{
    .name = "game",
    .width = 1280,
    .height = 720,
  };

  core::Engine<game::Game> engine{spec};
  engine.run();

  return 0;
}
