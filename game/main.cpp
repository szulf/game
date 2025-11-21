#include "engine/asset_manager.hpp"
#include "engine/engine.hpp"
#include "engine/event.hpp"
#include "engine/renderer/camera.hpp"
#include "engine/renderer/renderer.hpp"
#include "engine/renderer/scene.hpp"
#include "badtl/string.hpp"
#include "badtl/print.hpp"

namespace game {

struct Game {
  enum class Action {
    Interact,
    StartTest,
  };

  void init(btl::Allocator& allocator) {
    asset_manager = core::AssetManager::make(allocator);
    core::AssetManager::instance = &asset_manager;
    scene = core::Scene::make(
      core::Model::from_file(btl::String::make("assets/backpack.obj"), allocator).expect("couldnt parse model"),
      core::Camera::make(core::Camera::Type::Perspective, {0.0f, 0.0f, -5.0f}),
      allocator
    );
    key_map.bind(core::Key::E, Action::Interact);
    key_map.bind(core::Key::T, Action::StartTest);
  }

  void render() {
    core::renderer::clearScreen();
    core::renderer::render(scene);
  }

  void update(float) {
    scene.camera.yaw += 1.0f;
    scene.camera.updateCameraVectors();
  }

  void event(const core::Event& e) {
    switch (e.tag) {
      case core::Event::Tag::Resize: {
        scene.camera.viewport_width = e.event.resize.width;
        scene.camera.viewport_height = e.event.resize.height;
      } break;
      case core::Event::Tag::Keydown: {
        const auto action = key_map[e.event.keydown.key];
        switch (action) {
          case Action::Interact: {
            btl::print("interacted by pressing some key\n");
          } break;
          case Action::StartTest: {
            // TODO(szulf): somehow change the state to test
          } break;
        }
      } break;
      case core::Event::Tag::MouseMove: {
      } break;
    }
  }

  core::KeyMap<Action> key_map;
  core::Scene scene;
  core::AssetManager asset_manager;
};

struct Test {
  void init() {}
  void render() {}
  void update(float) {}
  void event(const core::Event&) {}
};

#if 0
struct GameStates {
  enum class Tag {
    Game,
    Test,
  };
  union States {
    Game game;
    Test test;
  };

  void init() {
    switch (tag) {
      case Tag::Game: {
        states.game.init();
      } break;
      case Tag::Test: {
        states.test.init();
      } break;
    }
  }

  void render() {
    switch (tag) {
      case Tag::Game: {
        states.game.render();
      } break;
      case Tag::Test: {
        states.test.render();
      } break;
    }
  }

  void update(float t) {
    switch (tag) {
      case Tag::Game: {
        states.game.update(t);
      } break;
      case Tag::Test: {
        states.test.update(t);
      } break;
    }
  }

  void event(const core::Event& event) {
    switch (tag) {
      case Tag::Game: {
        states.game.event(event);
      } break;
      case Tag::Test: {
        states.test.event(event);
      } break;
    }
  }

  States states;
  Tag tag;
};
#endif

}

btl::i32 main() {
  auto allocator = btl::Allocator::make(btl::Allocator::Type::Arena, GB(1));

  core::AppSpec spec = {
    "game",
    1280,
    720,
    allocator,
  };

  core::Engine<game::Game> engine{spec};
  engine.run();

  return 0;
}
