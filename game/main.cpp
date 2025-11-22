#include "engine/asset_manager.hpp"
#include "engine/engine.hpp"
#include "engine/event.hpp"
#include "engine/renderer/camera.hpp"
#include "engine/renderer/renderer.hpp"
#include "engine/renderer/scene.hpp"
#include "badtl/string.hpp"
#include "badtl/print.hpp"
#include "badtl/function.hpp"

namespace game {

enum class StateTag {
  Main,
  Test,
};

struct Main {
  enum class Action {
    Interact = 1,
    PushTest,
  };

  void init(
    const btl::Function<void, StateTag>& push_state_fn,
    const btl::Function<void, StateTag>& pop_state_fn,
    btl::Allocator& alloc
  ) {
    allocator = &alloc;
    asset_manager = core::AssetManager::make(*allocator);
    core::AssetManager::instance = &asset_manager;
    scene = core::Scene::make(
      core::Model::from_file(btl::String::make("assets/backpack.obj"), *allocator).expect("couldnt parse model"),
      core::Camera::make(core::Camera::Type::Perspective, {0.0f, 0.0f, -5.0f}, core::get_width(), core::get_height()),
      *allocator
    );
    key_map.bind(core::Key::E, Action::Interact);
    key_map.bind(core::Key::T, Action::PushTest);
    push_state = push_state_fn;
    pop_state = pop_state_fn;
  }

  void deinit() {}

  void render() const {
    core::renderer::clear_screen();
    core::renderer::render(scene);
  }

  void update(float) {
    scene.camera.yaw += 1.0f;
    scene.camera.update_camera_vectors();
  }

  void event(core::Event& e) {
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
          case Action::PushTest: {
            // TODO(szulf): pushing or poping state has to consume the event
            // probably will just do that myself lol
            push_state(StateTag::Test);
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
  btl::Function<void, StateTag> push_state;
  btl::Function<void, StateTag> pop_state;
  btl::Allocator* allocator;
};

struct Test {
  enum class Action {
    Pop = 1,
  };

  void init(
    const btl::Function<void, StateTag>& push_state_fn,
    const btl::Function<void, StateTag>& pop_state_fn,
    btl::Allocator&
  ) {
    push_state = push_state_fn;
    pop_state = pop_state_fn;
    key_map.bind(core::Key::E, Action::Pop);
    c = 0;
  }

  void deinit() {
    btl::println("done testing");
  }

  void render() const {}

  void update(float dt) {
    ++c;
    btl::println("{} beep boop running some tests {}", dt, c);
  }

  void event(core::Event& event) {
    switch (event.tag) {
      case core::Event::Tag::Resize:
        return;

      case core::Event::Tag::Keydown: {
        const auto action = key_map[event.event.keydown.key];
        switch (action) {
          case Action::Pop: {
            pop_state(StateTag::Test);
          } break;
        }
      } break;

      case core::Event::Tag::MouseMove:
        return;
    }
  }

  btl::u32 c;
  btl::Function<void, StateTag> push_state;
  btl::Function<void, StateTag> pop_state;
  core::KeyMap<Action> key_map;
};

struct States {
  union State {
    Main game;
    Test test;
  };
  struct StateEntry {
    State state;
    StateTag tag;
  };

  void init(btl::Allocator& alloc) {
    allocator = &alloc;
    states = btl::List<StateEntry>::make(5, *allocator);
    states.push({{}, StateTag::Main});

    // TODO(szulf): dont just set the state tag, probably queue it somehow, do i really??
    push_state = btl::Function<void, StateTag>{
      this,
      [](void* t, StateTag tag) {
        auto& states = *static_cast<States*>(t);
        states.states.push({{}, tag});
        states.dispatch_init(states.states[states.states.size - 1]);
      },
    };
    pop_state = btl::Function<void, StateTag>{
      this,
      [](void* t, StateTag tag) {
        auto& states = *static_cast<States*>(t);
        auto* found_elem = states.states.find_reverse_fn(
          btl::Function<bool, const StateEntry&>{
            &tag,
            [](void* tag, const StateEntry& entry) {
              return entry.tag == *static_cast<StateTag*>(tag);
            },
          }
        );
        for (btl::i32 i = static_cast<btl::i32>(states.states.size) - 1;
             i >= static_cast<btl::i32>(found_elem - states.states.begin());
             --i) {
          states.dispatch_deinit(states.states[static_cast<btl::usize>(i)]);
        }
        states.states.pop_to(found_elem);
      },
    };

    for (auto& entry : states) {
      dispatch_init(entry);
    }
  }

  void dispatch_init(StateEntry& entry) {
    switch (entry.tag) {
      case StateTag::Main: {
        entry.state.game.init(push_state, pop_state, *allocator);
      } break;
      case StateTag::Test: {
        entry.state.test.init(push_state, pop_state, *allocator);
      } break;
    }
  }

  void dispatch_deinit(StateEntry& entry) {
    switch (entry.tag) {
      case StateTag::Main: {
        entry.state.game.deinit();
      } break;
      case StateTag::Test: {
        entry.state.test.deinit();
      } break;
    }
  }

  void render() const {
    for (btl::i32 i = static_cast<btl::i32>(states.size) - 1; i >= 0; --i) {
      if (static_cast<btl::usize>(i) >= states.size) {
        continue;
      }
      auto& entry = states[static_cast<btl::usize>(i)];
      switch (entry.tag) {
        case StateTag::Main: {
          entry.state.game.render();
        } break;
        case StateTag::Test: {
          entry.state.test.render();
        } break;
      }
    }
  }

  void update(float t) {
    for (btl::i32 i = static_cast<btl::i32>(states.size) - 1; i >= 0; --i) {
      if (static_cast<btl::usize>(i) >= states.size) {
        continue;
      }
      auto& entry = states[static_cast<btl::usize>(i)];
      switch (entry.tag) {
        case StateTag::Main: {
          entry.state.game.update(t);
        } break;
        case StateTag::Test: {
          entry.state.test.update(t);
        } break;
      }
    }
  }

  void event(core::Event& event) {
    auto& entry = states[states.size - 1];
    switch (entry.tag) {
      case StateTag::Main: {
        entry.state.game.event(event);
      } break;
      case StateTag::Test: {
        entry.state.test.event(event);
      } break;
    }
  }

  btl::List<StateEntry> states;
  btl::Function<void, StateTag> push_state;
  btl::Function<void, StateTag> pop_state;
  btl::Allocator* allocator;
};

}

btl::i32 main() {
  auto allocator = btl::Allocator::make(btl::Allocator::Type::Arena, GB(1));

  core::AppSpec spec = {
    "game",
    1280,
    720,
    allocator,
  };

  auto engine = core::Engine<game::States>::make(spec);
  engine.run();

  return 0;
}
