#include "base/base.cpp"
#include "platform.h"

#include <SDL3/SDL.h>
#include "gl_functions.cpp"

static u32 g_width = 0;
GET_WIDTH_FN(get_width)
{
  return g_width;
}

static u32 g_height = 0;
GET_HEIGHT_FN(get_height)
{
  return g_height;
}

READ_FILE_FN(read_file)
{
  SDL_Storage* storage = SDL_OpenFileStorage(nullptr);
  ERROR_ASSERT(storage, *out_error, GLOBAL_ERROR_FILE_READING, nullptr);
  defer(SDL_CloseStorage(storage));

  auto file_size_success = SDL_GetStorageFileSize(storage, path, out_size);
  ERROR_ASSERT(file_size_success, *out_error, GLOBAL_ERROR_FILE_READING, nullptr);
  void* file = alloc(*allocator, *out_size);
  auto read_file_success = SDL_ReadStorageFile(storage, path, file, *out_size);
  ERROR_ASSERT(read_file_success, *out_error, GLOBAL_ERROR_FILE_READING, nullptr);
  return file;
}

WRITE_FILE_FN(write_file)
{
  SDL_Storage* storage = SDL_OpenFileStorage(nullptr);
  ERROR_ASSERT(storage, *out_error, GLOBAL_ERROR_FILE_WRITING, );
  defer(SDL_CloseStorage(storage));

  while (!SDL_StorageReady(storage))
  {
    SDL_Delay(1);
  }

  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));

  auto write_file_success = SDL_WriteStorageFile(
    storage,
    path,
    string_to_cstr(*string, scratch_arena.allocator),
    string->size
  );
  ERROR_ASSERT(write_file_success, *out_error, GLOBAL_ERROR_FILE_WRITING, );
}

static SDL_SharedObject* so;

GameAPI load_game_api()
{
  GameAPI out = {};
#ifdef OS_LINUX
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
    case KEY_E:
      return SDLK_E;
    case KEY_SPACE:
      return SDLK_SPACE;
    case KEY_LSHIFT:
      return SDLK_LSHIFT;
    case KEY_F1:
      return SDLK_F1;
    case KEY_F2:
      return SDLK_F2;
  }
  return (SDL_Keycode) -1;
}

i32 main()
{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  PlatformAPI platform_api = {};
  platform_api.read_file = read_file;
  platform_api.write_file = write_file;
  platform_api.get_width = get_width;
  platform_api.get_height = get_height;

  GameAPI game = load_game_api();

  GameSpec spec = {};
  game.spec(&spec);
  g_width = spec.width;
  g_width = spec.height;

  SDL_Window* window = SDL_CreateWindow(
    spec.name,
    (i32) spec.width,
    (i32) spec.height,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
  );
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
  RenderingAPI gl_api;
  setup_gl_functions(&gl_api);

#ifdef GAME_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debug_callback, nullptr);
#endif

  GameMemory memory = {};
  memory.size = spec.memory_size;
  memory.memory = malloc(memory.size);
  mem_set(memory.memory, 0, memory.size);

  SDL_PathInfo game_lib_info = {};
  SDL_GetPathInfo("./build/libgame.so", &game_lib_info);

  GameInput input = {};

  game.apis(&gl_api, &platform_api);
  game.init(&memory, &input);

  bool running = true;
  while (running)
  {
    auto start_ms = SDL_GetTicks();

    SDL_WarpMouseInWindow(window, (f32) g_width / 2.0f, (f32) g_height / 2.0f);

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

    for (usize i = 0; i < array_size(input.states); ++i)
    {
      input.states[i].transition_count = 0;
    }

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
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_KEY_DOWN:
        {
          for (usize i = 0; i < array_size(input.states); ++i)
          {
            if (e.key.key == sdlk_from_key(input.states[i].key))
            {
              ++input.states[i].transition_count;
            }
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
      input.move_front.ended_down =
        key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.move_front.key), nullptr)];
      input.move_back.ended_down =
        key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.move_back.key), nullptr)];
      input.move_left.ended_down =
        key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.move_left.key), nullptr)];
      input.move_right.ended_down =
        key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.move_right.key), nullptr)];

      input.interact.ended_down =
        key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.interact.key), nullptr)];

      input.camera_move_up.ended_down =
        key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.camera_move_up.key), nullptr)];
      input.camera_move_down.ended_down =
        key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.camera_move_down.key), nullptr)];

      input.toggle_camera_mode.ended_down =
        key_states[SDL_GetScancodeFromKey(sdlk_from_key(input.toggle_camera_mode.key), nullptr)];
      input.toggle_display_bounding_boxes.ended_down = key_states
        [SDL_GetScancodeFromKey(sdlk_from_key(input.toggle_display_bounding_boxes.key), nullptr)];
    }

    game.update(&memory, &input, 1.0f / (f32) FPS);

    game.render(&memory);
    SDL_GL_SwapWindow(window);

    auto end_ms = SDL_GetTicks();
    auto ms_in_frame = end_ms - start_ms;
    if (ms_in_frame < MSPF)
    {
      SDL_Delay((u32) (MSPF - ms_in_frame));
    }
  }

  return 0;
}
