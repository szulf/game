#include <print>
#include <utility>
#include <filesystem>

#include "engine.hpp"
#include "renderer.hpp"

namespace utils
{

auto setupSimpleScene(const std::filesystem::path& obj_path) -> core::Scene
{
  (void) obj_path;
  return {};
}

}

namespace game
{

class AppLayer : public core::Layer
{
  virtual auto onRender() -> void override
  {
    core::renderer::clearScreen();
    const auto scene{utils::setupSimpleScene("assets/cube.obj")};
    core::renderer::render(scene);
  }

  // virtual auto onUpdate(float dt) -> void override {}
  // virtual auto onEvent() -> void override {}
};

}

auto main() -> std::int32_t
{
  core::AppSpec spec{
    .name = "game",
    .width = 640,
    .height = 480,
  };

  core::Engine engine{spec};
  engine.pushLayer<game::AppLayer>();
  engine.run();
  return 0;
}
