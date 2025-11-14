#pragma once

#include <thread>
#ifdef GAME_SDL3
#  include <SDL3/SDL.h>
#endif

#include <variant>
#include <atomic>

#include "event.hpp"
#include "renderer/renderer.hpp"
#include "utils/templates.hpp"

namespace core {

struct AppSpec final {
  const char* name{};
  std::int32_t width{};
  std::int32_t height{};
};

struct NullStarterState {
  using PrevStates = utils::type_list<>;
  void render() {}
  void update(float) {}
  void event(const Event&) {}
};

template <typename... Ts>
struct Engine final {
  using StarterState = utils::get_first_type<Ts...>::type;

public:
#ifdef GAME_SDL3
  struct PlatformData {
    SDL_Window* window;
    SDL_GLContext gl_context;
  };
#else
#  error Unknown platform
#endif

public:
  Engine(const AppSpec& spec);

  template <typename NewState>
  void setState();

  void run();

  template <typename State>
  void updateThread(State& state);

public:
  std::atomic<bool> running{true};

  std::mutex state_mutex{};
  std::variant<NullStarterState, Ts...> states{NullStarterState{}};

  std::mutex main_thread_queue_mutex{};
  std::vector<std::function<void()>> main_thread_queue{};

  PlatformData platform_data{};

  static constexpr auto TPS{20.0f};
  static constexpr auto MSPT{std::chrono::seconds(1) / TPS};
};

}

#ifdef GAME_OPENGL
// TODO(szulf): move this another file and just include it here

#  include "renderer/gl_functions.hpp"

namespace core {

template <typename... Ts>
Engine<Ts...>::Engine(const AppSpec& spec) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  platform_data.window = SDL_CreateWindow(spec.name, spec.width, spec.height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  ASSERT(platform_data.window, "failed to create sdl3 window");

#  ifdef GAME_DEBUG
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#  else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#  endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  platform_data.gl_context = SDL_GL_CreateContext(platform_data.window);
  ASSERT(platform_data.gl_context, "failed to create sdl3 opengl context");
  setupGLFunctions();

#  ifdef GAME_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debugCallback, nullptr);
#  endif

  states = StarterState{};
  renderer::init();
}

template <typename... Ts>
template <typename NewState>
void Engine<Ts...>::setState() {
  std::visit([&](const auto& val) {
    using CurrentState = std::decay_t<decltype(val)>;
    static_assert(
      utils::type_list_contains<typename NewState::PrevStates, CurrentState>,
      "NewState has to be in the CurrentStates PrevStates declaration"
    );
    states = NewState{};
  }, states);
}

template <typename... Ts>
void Engine<Ts...>::run() {
  std::visit([&](auto& state) {
    std::jthread update_thread{[&]() {
      updateThread(state);
    }};

    while (running) {
      {
        std::lock_guard lock{main_thread_queue_mutex};
        for (const auto& callable : main_thread_queue) {
          callable();
        }
        main_thread_queue.clear();
      }

      {
        std::lock_guard lock{state_mutex};
        state.render();
      }
      SDL_GL_SwapWindow(platform_data.window);
    }
  }, states);
}

template <typename... Ts>
template <typename State>
void Engine<Ts...>::updateThread(State& state) {
  SDL_Event e{};
  while (running) {
    const auto ms{std::chrono::high_resolution_clock::now()};

    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_EVENT_QUIT: {
          running = false;
        } break;
        case SDL_EVENT_WINDOW_RESIZED: {
          {
            std::lock_guard lock{main_thread_queue_mutex};
            main_thread_queue.push_back([width = e.window.data1, height = e.window.data2]() {
              glViewport(0, 0, width, height);
            });
          }
          std::lock_guard lock{state_mutex};
          state.event(
            WindowResizeEvent{static_cast<std::uint32_t>(e.window.data1), static_cast<std::uint32_t>(e.window.data2)}
          );
        } break;
      }
    }

    {
      std::lock_guard lock{state_mutex};
      state.update(0.0f);
    }

    const auto end_ms{std::chrono::high_resolution_clock::now()};
    const auto ms_in_frame = end_ms - ms;
    if (ms_in_frame < MSPT) {
      std::this_thread::sleep_for(MSPT - ms_in_frame);
    }
  }
}

}

#else
#  error Unknown rendering backend
#endif
