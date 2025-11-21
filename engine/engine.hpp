#pragma once

#ifdef GAME_SDL3
#  include <SDL3/SDL.h>
#endif

#include "engine/event.hpp"
#include "engine/renderer/renderer.hpp"
#include "badtl/utils.hpp"
#include "badtl/threads.hpp"
#include "badtl/time.hpp"
#include "badtl/allocator.hpp"
#include "badtl/list.hpp"
#include "badtl/function.hpp"

namespace core {

struct AppSpec final {
  const char* name;
  btl::i32 width;
  btl::i32 height;
  btl::Allocator& allocator;
};

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

  Engine(AppSpec& spec);

  void run();
  btl::ThreadFunction update;

  btl::Allocator& allocator;

  btl::AtomicBool running;

  btl::Mutex state_mutex;
  T game;

  btl::Mutex main_thread_queue_mutex;
  btl::List<btl::Function> main_thread_queue;

  PlatformData platform_data;

  static constexpr btl::u32 TPS = 20;
  static constexpr btl::u64 MSPT = 1000 / TPS;
};

}

#ifdef GAME_OPENGL

#  include "renderer/gl_functions.hpp"

namespace core {

namespace {

inline static Key keyFromSDLK(SDL_Keycode sdlk) {
  switch (sdlk) {
    case SDLK_E:
      return Key::E;
  }

  return static_cast<Key>(-1);
}

struct WidthHeight {
  btl::i32 width;
  btl::i32 height;
};

}

template <typename T>
Engine<T>::Engine(AppSpec& spec) : allocator{spec.allocator} {
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

  renderer::init();
  game.init(allocator);

  running.set(true);
  state_mutex = btl::Mutex::make();
  main_thread_queue_mutex = btl::Mutex::make();

  main_thread_queue = btl::List<btl::Function>::make(16, allocator);

  update = [](void* data) {
    Engine& engine = *static_cast<Engine*>(data);
    SDL_Event e = {};
    while (engine.running.get()) {
      btl::u64 ms = btl::time::now();

      while (SDL_PollEvent(&e)) {
        switch (e.type) {
          case SDL_EVENT_QUIT: {
            engine.running.set(false);
          } break;
          case SDL_EVENT_WINDOW_RESIZED: {
            WidthHeight* width_height = static_cast<WidthHeight*>(engine.allocator.alloc(sizeof(WidthHeight)));
            width_height->width = e.window.data1;
            width_height->height = e.window.data2;
            engine.main_thread_queue_mutex.lock();
            engine.main_thread_queue.push(
              btl::Function{
                [](void* args) -> void* {
                  WidthHeight wh = *static_cast<WidthHeight*>(args);
                  glViewport(0, 0, wh.width, wh.height);
                  return nullptr;
                },
                width_height
              }
            );
            engine.main_thread_queue_mutex.unlock();

            engine.state_mutex.lock();
            engine.game.event(
              Event::make(ResizeEvent{static_cast<btl::u32>(e.window.data1), static_cast<btl::u32>(e.window.data2)})
            );
            engine.state_mutex.unlock();
          } break;
          case SDL_EVENT_KEY_DOWN: {
            if (keyFromSDLK(e.key.key) == static_cast<Key>(-1)) {
              continue;
            }
            engine.game.event(Event::make(KeydownEvent{keyFromSDLK(e.key.key)}));
          } break;
          case SDL_EVENT_MOUSE_MOTION: {
            engine.game.event(
              Event::make(MouseMoveEvent{static_cast<btl::u32>(e.motion.x), static_cast<btl::u32>(e.motion.y)})
            );
          } break;
        }
      }

      engine.state_mutex.lock();
      engine.game.update(0.0f);
      engine.state_mutex.unlock();

      btl::u64 end_ms = btl::time::now();
      btl::u64 ms_in_frame = end_ms - ms;
      if (ms_in_frame < MSPT) {
        btl::time::sleep_ms(static_cast<btl::u32>(MSPT - ms_in_frame));
      }
    }
    return 0;
  };
}

template <typename T>
void Engine<T>::run() {
  auto update_thread = btl::Thread::make(update, this);
  update_thread.detach();

  while (running.get()) {
    main_thread_queue_mutex.lock();
    for (const auto& fn : main_thread_queue) {
      fn();
    }
    main_thread_queue.clear();
    main_thread_queue_mutex.unlock();

    state_mutex.lock();
    game.render();
    state_mutex.unlock();
    SDL_GL_SwapWindow(platform_data.window);
  }
}

}

#else
#  error Unknown rendering backend
#endif
