#include "os.h"

#include "SDL3/SDL_mouse.h"
#include "base/base.h"
#include "sdl3/include/SDL3/SDL.h"

#include "gl_functions.h"

namespace os
{

void init()
{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
}

const char* key_to_cstr(Key key)
{
  switch (key)
  {
    case Key::A:
      return "A";
    case Key::B:
      return "B";
    case Key::C:
      return "C";
    case Key::D:
      return "D";
    case Key::E:
      return "E";
    case Key::F:
      return "F";
    case Key::G:
      return "G";
    case Key::H:
      return "H";
    case Key::I:
      return "I";
    case Key::J:
      return "J";
    case Key::K:
      return "K";
    case Key::L:
      return "L";
    case Key::M:
      return "M";
    case Key::N:
      return "N";
    case Key::O:
      return "O";
    case Key::P:
      return "P";
    case Key::Q:
      return "Q";
    case Key::R:
      return "R";
    case Key::S:
      return "S";
    case Key::T:
      return "T";
    case Key::U:
      return "U";
    case Key::V:
      return "V";
    case Key::W:
      return "W";
    case Key::X:
      return "X";
    case Key::Y:
      return "Y";
    case Key::Z:
      return "Z";
    case Key::F1:
      return "F1";
    case Key::F2:
      return "F2";
    case Key::F3:
      return "F3";
    case Key::F4:
      return "F4";
    case Key::F5:
      return "F5";
    case Key::F6:
      return "F6";
    case Key::F7:
      return "F7";
    case Key::F8:
      return "F8";
    case Key::F9:
      return "F9";
    case Key::F10:
      return "F10";
    case Key::F11:
      return "F11";
    case Key::F12:
      return "F12";
    case Key::SPACE:
      return "SPACE";
    case Key::LSHIFT:
      return "LSHIFT";
    default:
    case Key::COUNT:
      return "invalid key";
  }
}

Key string_to_key(const String& str, Error& out_error)
{
  if (str == "A")
  {
    return Key::A;
  }
  else if (str == "B")
  {
    return Key::B;
  }
  else if (str == "C")
  {
    return Key::C;
  }
  else if (str == "D")
  {
    return Key::D;
  }
  else if (str == "E")
  {
    return Key::E;
  }
  else if (str == "F")
  {
    return Key::F;
  }
  else if (str == "G")
  {
    return Key::G;
  }
  else if (str == "H")
  {
    return Key::H;
  }
  else if (str == "I")
  {
    return Key::I;
  }
  else if (str == "J")
  {
    return Key::J;
  }
  else if (str == "K")
  {
    return Key::K;
  }
  else if (str == "L")
  {
    return Key::L;
  }
  else if (str == "M")
  {
    return Key::M;
  }
  else if (str == "N")
  {
    return Key::N;
  }
  else if (str == "O")
  {
    return Key::O;
  }
  else if (str == "P")
  {
    return Key::P;
  }
  else if (str == "Q")
  {
    return Key::Q;
  }
  else if (str == "R")
  {
    return Key::R;
  }
  else if (str == "S")
  {
    return Key::S;
  }
  else if (str == "T")
  {
    return Key::T;
  }
  else if (str == "U")
  {
    return Key::U;
  }
  else if (str == "V")
  {
    return Key::V;
  }
  else if (str == "W")
  {
    return Key::W;
  }
  else if (str == "X")
  {
    return Key::X;
  }
  else if (str == "Y")
  {
    return Key::Y;
  }
  else if (str == "Z")
  {
    return Key::Z;
  }
  else if (str == "F1")
  {
    return Key::F1;
  }
  else if (str == "F2")
  {
    return Key::F2;
  }
  else if (str == "F3")
  {
    return Key::F3;
  }
  else if (str == "F4")
  {
    return Key::F4;
  }
  else if (str == "F5")
  {
    return Key::F5;
  }
  else if (str == "F6")
  {
    return Key::F6;
  }
  else if (str == "F7")
  {
    return Key::F7;
  }
  else if (str == "F8")
  {
    return Key::F8;
  }
  else if (str == "F9")
  {
    return Key::F9;
  }
  else if (str == "F10")
  {
    return Key::F10;
  }
  else if (str == "F11")
  {
    return Key::F11;
  }
  else if (str == "F12")
  {
    return Key::F12;
  }
  else if (str == "SPACE")
  {
    return Key::SPACE;
  }
  else if (str == "LSHIFT")
  {
    return Key::LSHIFT;
  }

  out_error = "Invalid key string.";
  return (Key) 0;
}

Window Window::open(const char* name, Dimensions dimensions)
{
  Window out = {};
  out.name = name;
  out.dimensions = dimensions;

  out.handle = SDL_CreateWindow(
    name,
    (i32) out.dimensions.width,
    (i32) out.dimensions.height,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
  );
  ASSERT(out.handle, "Failed to open window.");
  out.running = true;

  return out;
}

void Window::init_rendering_api()
{
#ifdef GAME_DEBUG
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GLContext gl_context = SDL_GL_CreateContext((SDL_Window*) handle);
  ASSERT(gl_context, "failed to create sdl3 opengl context");
  setup_gl_functions();

#ifdef GAME_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debug_callback, nullptr);
#endif
}

static SDL_Keycode sdlk_from_key(Key key)
{
  switch (key)
  {
    case Key::A:
      return SDLK_A;
    case Key::B:
      return SDLK_B;
    case Key::C:
      return SDLK_C;
    case Key::D:
      return SDLK_D;
    case Key::E:
      return SDLK_E;
    case Key::F:
      return SDLK_F;
    case Key::G:
      return SDLK_G;
    case Key::H:
      return SDLK_H;
    case Key::I:
      return SDLK_I;
    case Key::J:
      return SDLK_J;
    case Key::K:
      return SDLK_K;
    case Key::L:
      return SDLK_L;
    case Key::M:
      return SDLK_M;
    case Key::N:
      return SDLK_N;
    case Key::O:
      return SDLK_O;
    case Key::P:
      return SDLK_P;
    case Key::Q:
      return SDLK_Q;
    case Key::R:
      return SDLK_R;
    case Key::S:
      return SDLK_S;
    case Key::T:
      return SDLK_T;
    case Key::U:
      return SDLK_U;
    case Key::V:
      return SDLK_V;
    case Key::W:
      return SDLK_W;
    case Key::X:
      return SDLK_X;
    case Key::Y:
      return SDLK_Y;
    case Key::Z:
      return SDLK_Z;
    case Key::F1:
      return SDLK_F1;
    case Key::F2:
      return SDLK_F2;
    case Key::F3:
      return SDLK_F3;
    case Key::F4:
      return SDLK_F4;
    case Key::F5:
      return SDLK_F5;
    case Key::F6:
      return SDLK_F6;
    case Key::F7:
      return SDLK_F7;
    case Key::F8:
      return SDLK_F8;
    case Key::F9:
      return SDLK_F9;
    case Key::F10:
      return SDLK_F10;
    case Key::F11:
      return SDLK_F11;
    case Key::F12:
      return SDLK_F12;
    case Key::SPACE:
      return SDLK_SPACE;
    case Key::LSHIFT:
      return SDLK_LSHIFT;
    case Key::COUNT:
    default:
      return (SDL_Keycode) -1;
  }
}

static Key key_from_sdlk(SDL_Keycode sdlk)
{
  switch (sdlk)
  {
    case SDLK_A:
      return Key::A;
    case SDLK_B:
      return Key::B;
    case SDLK_C:
      return Key::C;
    case SDLK_D:
      return Key::D;
    case SDLK_E:
      return Key::E;
    case SDLK_F:
      return Key::F;
    case SDLK_G:
      return Key::G;
    case SDLK_H:
      return Key::H;
    case SDLK_I:
      return Key::I;
    case SDLK_J:
      return Key::J;
    case SDLK_K:
      return Key::K;
    case SDLK_L:
      return Key::L;
    case SDLK_M:
      return Key::M;
    case SDLK_N:
      return Key::N;
    case SDLK_O:
      return Key::O;
    case SDLK_P:
      return Key::P;
    case SDLK_Q:
      return Key::Q;
    case SDLK_R:
      return Key::R;
    case SDLK_S:
      return Key::S;
    case SDLK_T:
      return Key::T;
    case SDLK_U:
      return Key::U;
    case SDLK_V:
      return Key::V;
    case SDLK_W:
      return Key::W;
    case SDLK_X:
      return Key::X;
    case SDLK_Y:
      return Key::Y;
    case SDLK_Z:
      return Key::Z;
    case SDLK_F1:
      return Key::F1;
    case SDLK_F2:
      return Key::F2;
    case SDLK_F3:
      return Key::F3;
    case SDLK_F4:
      return Key::F4;
    case SDLK_F5:
      return Key::F5;
    case SDLK_F6:
      return Key::F6;
    case SDLK_F7:
      return Key::F7;
    case SDLK_F8:
      return Key::F8;
    case SDLK_F9:
      return Key::F9;
    case SDLK_F10:
      return Key::F10;
    case SDLK_F11:
      return Key::F11;
    case SDLK_F12:
      return Key::F12;
    case SDLK_SPACE:
      return Key::SPACE;
    case SDLK_LSHIFT:
      return Key::LSHIFT;
    default:
      return (Key) -1;
  }
}

void Window::update()
{
  SDL_Event e;
  while (SDL_PollEvent(&e))
  {
    switch (e.type)
    {
      case SDL_EVENT_QUIT:
      {
        running = false;
      }
      break;
      case SDL_EVENT_WINDOW_RESIZED:
      {
        dimensions.width = (u32) e.window.data1;
        dimensions.height = (u32) e.window.data2;
      }
      break;
      case SDL_EVENT_KEY_UP:
      case SDL_EVENT_KEY_DOWN:
      {
        ++input.states[key_from_sdlk(e.key.key)].transition_count;
      }
      break;
      case SDL_EVENT_MOUSE_MOTION:
      {
        input.mouse_pos = {e.motion.x, e.motion.y};
        input.mouse_delta = {e.motion.xrel, e.motion.yrel};
      }
      break;
    }
  }

  const bool* key_states = SDL_GetKeyboardState(nullptr);
  for (usize i = 0; i < input.states.size; ++i)
  {
    input.states.data[i].ended_down =
      key_states[SDL_GetScancodeFromKey(sdlk_from_key((Key) i), nullptr)];
  }
}

void Window::clear_input()
{
  for (usize i = 0; i < input.states.size; ++i)
  {
    input.states.data[i].transition_count = 0;
  }
}

void Window::swap_buffers()
{
  SDL_GL_SwapWindow((SDL_Window*) handle);
}

void Window::consume_mouse_pointer()
{
  SDL_HideCursor();
  SDL_WarpMouseInWindow(
    (SDL_Window*) handle,
    (f32) dimensions.width / 2.0f,
    (f32) dimensions.height / 2.0f
  );
}

void Window::release_mouse_pointer()
{
  SDL_ShowCursor();
}

void* read(const char* path, Allocator& allocator, usize& out_size, Error& out_error)
{
  SDL_Storage* storage = SDL_OpenFileStorage(nullptr);
  ERROR_ASSERT(storage, out_error, "File reading error.", nullptr);
  defer(SDL_CloseStorage(storage));

  auto file_size_success = SDL_GetStorageFileSize(storage, path, &out_size);
  ERROR_ASSERT(file_size_success, out_error, "File reading error.", nullptr);
  void* file = allocator.alloc(out_size);
  auto read_file_success = SDL_ReadStorageFile(storage, path, file, out_size);
  ERROR_ASSERT(read_file_success, out_error, "File reading error.", nullptr);
  return file;
}

String read_to_string(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  usize size;
  void* file = read(path, allocator, size, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  return String::make((const char*) file, size);
}

f32 time_now()
{
  return (f32) SDL_GetTicks() / 1000.0f;
}

void* alloc(usize bytes)
{
  void* ptr = SDL_malloc(bytes);
  mem_set(ptr, 0, bytes);
  return ptr;
}

void free(void* ptr)
{
  return SDL_free(ptr);
}
}
