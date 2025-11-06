#include <print>
#include <utility>
#include <filesystem>

#include "engine.hpp"

namespace game
{

class AppLayer : public core::Layer
{
  virtual auto onRender() -> void override {}
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
