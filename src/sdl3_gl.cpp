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
  void* file = alloc(*allocator, *out_size);
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
  out.post_reload = (PostReloadFN*) SDL_LoadFunction(so, "post_reload");
  out.render = (RenderFN*) SDL_LoadFunction(so, "render");
  out.update = (UpdateFN*) SDL_LoadFunction(so, "update");
  out.event = (EventFN*) SDL_LoadFunction(so, "event");

  return out;
}

static Key key_from_sdlk(SDL_Keycode key)
{
  switch (key)
  {
    case SDLK_W:
      return KEY_W;
    case SDLK_S:
      return KEY_S;
    case SDLK_A:
      return KEY_A;
    case SDLK_D:
      return KEY_D;
    case SDLK_SPACE:
      return KEY_SPACE;
    case SDLK_LSHIFT:
      return KEY_LSHIFT;
    case SDLK_F1:
      return KEY_F1;
  }
  return (Key) 0;
}

static SDL_Keycode sdlk_from_key(Key key)
{
  switch (key)
  {
    case KEY_W:
      return SDLK_W;
    case KEY_S:
      return SDLK_S;
    case KEY_A:
      return SDLK_A;
    case KEY_D:
      return SDLK_D;
    case KEY_SPACE:
      return SDLK_SPACE;
    case KEY_LSHIFT:
      return SDLK_LSHIFT;
    case KEY_F1:
      return SDLK_F1;
    case KEY_COUNT:
    {
      ASSERT(false, "this should never happen");
    }
    break;
  }
  return (SDL_Keycode) -1;
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
  SDL_HideCursor();

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

  GameInput input = {};

  SDL_PathInfo game_lib_info = {};
  SDL_GetPathInfo("./build/libgame.so", &game_lib_info);

  game.apis(&gl_api, &platform_api);
  game.init(&memory, &input);

  bool running = true;
  u64 accumulator = 0;
  auto last_time = SDL_GetTicks();
  while (running)
  {
    auto time = SDL_GetTicks();
    auto dt = time - last_time;
    last_time = time;
    accumulator += dt;

    SDL_WarpMouseInWindow(window, (f32) g_width / 2.0f, (f32) g_height / 2.0f);

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
      game.post_reload(&memory);
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
          Event event = {};
          event.type = EVENT_TYPE_WINDOW_RESIZE;
          event.data.window_resize = {g_width, g_height};
          game.event(&memory, &event);
        }
        break;
        case SDL_EVENT_KEY_DOWN:
        {
          if (e.key.key == sdlk_from_key(input.toggle_debug_mode_key))
          {
            input.toggle_debug_mode = true;
          }
        }
        break;
        case SDL_EVENT_MOUSE_MOTION:
        {
          input.mouse_relative = {e.motion.xrel, e.motion.yrel};
        }
        break;
      }
    }

    {
      const bool* key_states = SDL_GetKeyboardState(nullptr);
      if (key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.move_front_key), nullptr)])
      {
        input.move.z = -1.0f;
      }
      if (key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.move_back_key), nullptr)])
      {
        input.move.z = 1.0f;
      }
      if (key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.move_left_key), nullptr)])
      {
        input.move.x = -1.0f;
      }
      if (key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.move_right_key), nullptr)])
      {
        input.move.x = 1.0f;
      }
      input.move = vec3_normalize(input.move);
    }

    while (accumulator >= MSPT)
    {
      game.update(&memory, &input, 1.0f / TPS);
      accumulator -= MSPT;

      input.move = {};
      input.toggle_debug_mode = false;
      input.mouse_pos_last = input.mouse_pos;
    }

    game.render(&memory);
    SDL_GL_SwapWindow(window);
  }

  return 0;
}
