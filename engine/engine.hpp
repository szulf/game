#pragma once

#ifdef GAME_SDL3
#  include <SDL3/SDL.h>
#endif

#include "engine/renderer/gl_functions.hpp"
#include "engine/event.hpp"
#include "engine/renderer/renderer.hpp"
#include "badtl/utils.hpp"
#include "badtl/time.hpp"

namespace core {

// TODO(szulf): move these somewhere in the future?
static btl::u32 g_width = 0;
static btl::u32 g_height = 0;
inline btl::u32 get_width() {
  return g_width;
}
inline btl::u32 get_height() {
  return g_height;
}

struct AppSpec final {
  const char* name;
  btl::u32 width;
  btl::u32 height;
  btl::Allocator& allocator;
};

inline static Key keyFromSDLK(SDL_Keycode sdlk) {
  switch (sdlk) {
    case SDLK_E:
      return Key::E;
    case SDLK_T:
      return Key::T;
  }

  return static_cast<Key>(-1);
}

template <typename T>
struct Engine {
#ifdef GAME_SDL3
  struct PlatformData {
    SDL_Window* window;
    SDL_GLContext gl_context;
  };
#else
#  error Unknown platform
#endif

  static Engine<T> make(AppSpec& spec) {
    g_width = spec.width;
    g_width = spec.height;
    Engine<T> out = {};
    out.allocator = &spec.allocator;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    out.platform_data.window = SDL_CreateWindow(
      spec.name,
      static_cast<btl::i32>(spec.width),
      static_cast<btl::i32>(spec.height),
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
    );
    ASSERT(out.platform_data.window, "failed to create sdl3 window");

#ifdef GAME_DEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    out.platform_data.gl_context = SDL_GL_CreateContext(out.platform_data.window);
    ASSERT(out.platform_data.gl_context, "failed to create sdl3 opengl context");
    setup_gl_functions();

#ifdef GAME_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debug_callback, nullptr);
#endif

    renderer::init();
    out.game.init(*out.allocator);

    out.running = true;
    return out;
  }

  void run() {
    btl::u64 accumulator = 0;
    auto last_time = btl::time::now();
    while (running) {
      auto time = btl::time::now();
      auto dt = time - last_time;
      last_time = time;
      accumulator += dt;

      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        switch (e.type) {
          case SDL_EVENT_QUIT: {
            // TODO(szulf): could just exit the loop here, and not update/render the next frame
            running = false;
          } break;
          case SDL_EVENT_WINDOW_RESIZED: {
            g_width = static_cast<btl::u32>(e.window.data1);
            g_height = static_cast<btl::u32>(e.window.data2);
            glViewport(0, 0, e.window.data1, e.window.data2);
            auto event = Event::make(ResizeEvent{g_width, g_height});
            game.event(event);
          } break;
          case SDL_EVENT_KEY_DOWN: {
            if (keyFromSDLK(e.key.key) == static_cast<Key>(-1)) {
              continue;
            }
            auto event = Event::make(KeydownEvent{keyFromSDLK(e.key.key)});
            game.event(event);
          } break;
          case SDL_EVENT_MOUSE_MOTION: {
            auto event =
              Event::make(MouseMoveEvent{static_cast<btl::u32>(e.motion.x), static_cast<btl::u32>(e.motion.y)});
            game.event(event);
          } break;
        }
      }

      while (accumulator >= MSPT) {
        game.update(MSPT / 1000.0f);
        accumulator -= MSPT;
      }

      game.render();
      SDL_GL_SwapWindow(platform_data.window);
    }
  }

  bool running;
  T game;
  PlatformData platform_data;
  btl::Allocator* allocator;

  static constexpr btl::u32 TPS = 20;
  static constexpr btl::u64 MSPT = 1000 / TPS;
};

}
