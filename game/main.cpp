#include <print>

#include "engine.hpp"
#include "event.hpp"
#include "renderer/camera.hpp"
#include "renderer/renderer.hpp"
#include "renderer/scene.hpp"
#include "utils/templates.hpp"

namespace game {

struct Game {
  using PrevStates = utils::type_list<>;

  Game()
    : scene{
        {"assets/backpack.obj"},
        core::Camera{core::Camera::Type::Perspective, {0.0f, 0.0f, -5.0f}}
  } {}

  void render() {
    core::renderer::clearScreen();
    core::renderer::render(scene);
  }

  void update(float) {
    scene.camera.yaw += 1.0f;
    scene.camera.updateCameraVectors();
    std::println("{}", scene.camera.yaw);
  }

  void event(const core::Event& event) {
    switch (event.type) {
      using enum core::Event::Type;
      case WindowResize: {
        const auto e{static_cast<const core::WindowResizeEvent&>(event)};
        scene.camera.viewport_width = e.width;
        scene.camera.viewport_height = e.height;
      } break;
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
