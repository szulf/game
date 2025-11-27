#include "platform.h"

#include <SDL3/SDL.h>
#include "gl_functions.cpp"

static u32 g_width = 0;
u32 get_width()
{
  return g_width;
}
static u32 g_height = 0;
u32 get_height()
{
  return g_height;
}

void* read_file(const char* filepath, Allocator* allocator, usize* out_size)
{
  SDL_Storage* storage = SDL_OpenFileStorage(nullptr);
  defer(SDL_CloseStorage(storage));

  if (!SDL_GetStorageFileSize(storage, filepath, out_size))
  {
    return nullptr;
  }
  void* file = alloc(allocator, *out_size);
  if (!SDL_ReadStorageFile(storage, filepath, file, *out_size))
  {
    return nullptr;
  }
  return file;
}

static SDL_SharedObject* so;

GameAPI load_game_api()
{
  GameAPI out = {};
#ifdef PLATFORM_LINUX
  const char* lib_name = "./build/libgame.so";
#endif
  so = SDL_LoadObject(lib_name);
  ASSERT(so, "failed to load object %s\n", SDL_GetError());

  out.spec = (SpecFN*) SDL_LoadFunction(so, "spec");
  out.apis = (APIsFN*) SDL_LoadFunction(so, "apis");
  out.init = (InitFN*) SDL_LoadFunction(so, "init");
  out.reinit = (ReInitFN*) SDL_LoadFunction(so, "reinit");
  out.render = (RenderFN*) SDL_LoadFunction(so, "render");
  out.update = (UpdateFN*) SDL_LoadFunction(so, "update");
  out.event = (EventFN*) SDL_LoadFunction(so, "event");

  return out;
}

i32 main()
{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  PlatformAPI platform_api = {};
  platform_api.read_file = read_file;
  platform_api.get_width = get_width;
  platform_api.get_height = get_height;

  GameAPI game = load_game_api();

  GameSpec spec = {};
  game.spec(&spec);
  g_width = spec.width;
  g_width = spec.height;

  SDL_Window* window =
    SDL_CreateWindow(spec.name, (i32) spec.width, (i32) spec.height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  ASSERT(window, "failed to create sdl3 window");

#ifdef GAME_DEBUG
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  ASSERT(gl_context, "failed to create sdl3 opengl context");
  OpenGLAPI gl_api;
  setup_gl_functions(&gl_api);

#ifdef GAME_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debug_callback, nullptr);
#endif

  GameMemory memory = {};
  memory.size = spec.memory_size;
  memory.memory = malloc(memory.size);

  SDL_PathInfo game_lib_info = {};
  SDL_GetPathInfo("./build/libgame.so", &game_lib_info);

  game.apis(&gl_api, &platform_api);
  game.init(&memory);

  b8 running = true;
  u64 accumulator = 0;
  auto last_time = SDL_GetTicks();
  while (running)
  {
    auto time = SDL_GetTicks();
    auto dt = time - last_time;
    last_time = time;
    accumulator += dt;

#ifdef MODE_DEBUG
    // TODO(szulf): maybe dont do this every frame, but once every 5/10 frames or so
    SDL_PathInfo new_game_lib_info = {};
    SDL_GetPathInfo("./build/libgame.so", &new_game_lib_info);
    if (new_game_lib_info.modify_time > game_lib_info.modify_time)
    {
      // NOTE(szulf): magic delay to make this work
      SDL_Delay(100);
      if (so)
      {
        SDL_UnloadObject(so);
      }
      game = load_game_api();
      game.apis(&gl_api, &platform_api);
      game.reinit(&memory);
      game_lib_info = new_game_lib_info;
    }
#endif

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
      switch (e.type)
      {
        case SDL_EVENT_QUIT:
        {
          // TODO(szulf): could just exit the loop here, and not update/render the next frame
          running = false;
        }
        break;
        case SDL_EVENT_WINDOW_RESIZED:
        {
          g_width = (u32) e.window.data1;
          g_height = (u32) e.window.data2;
          glViewport(0, 0, e.window.data1, e.window.data2);
          Event event = {g_width, g_height};
          game.event(&memory, &event);
        }
        break;
        case SDL_EVENT_KEY_DOWN:
        {
          // if (key_from_sdlk(e.key.key) == static_cast<Key>(-1))
          // {
          //   continue;
          // }
          // auto event = Event::make(KeydownEvent{key_from_sdlk(e.key.key)});
          // game.event(&memory, &event);
        }
        break;
        case SDL_EVENT_MOUSE_MOTION:
        {
          // auto event = Event::make(MouseMoveEvent{static_cast<u32>(e.motion.x), static_cast<u32>(e.motion.y)});
          // game.event(&memory, &event);
        }
        break;
      }
    }

    while (accumulator >= MSPT)
    {
      game.update(&memory, 1.0f / TPS);
      accumulator -= MSPT;
    }

    game.render(&memory);
    SDL_GL_SwapWindow(window);
  }

  return 0;
}
