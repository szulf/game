#include "game.cpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#ifdef GAME_OPENGL
#  include "sdl3_ogl_functions.cpp"
#endif
#include "sdl3_game.h"

// TODO(szulf): set better starting dimensions
std::pair<i32, i32> dimensions = {640, 480};

static std::pair<i32, i32>
get_window_dimensions()
{
  return dimensions;
}

#ifdef GAME_DEBUG
static void APIENTRY
debug_callback(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* user
)
{
  UNUSED(length);
  UNUSED(user);

  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
  {
    return;
  }
  const char* source_str;
  switch (source)
  {
    case GL_DEBUG_SOURCE_API:
      source_str = "API";
      break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      source_str = "Window System";
      break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
      source_str = "Shader Compiler";
      break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
      source_str = "Third Party";
      break;
    case GL_DEBUG_SOURCE_APPLICATION:
      source_str = "Application";
      break;
    case GL_DEBUG_SOURCE_OTHER:
      source_str = "Other";
      break;
    default:
      source_str = "Unknown";
      break;
  }

  const char* type_str;
  switch (type)
  {
    case GL_DEBUG_TYPE_ERROR:
      type_str = "Error";
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      type_str = "Deprecated Behaviour";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      type_str = "Undefined Behaviour";
      break;
    case GL_DEBUG_TYPE_PORTABILITY:
      type_str = "Portability";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE:
      type_str = "Performance";
      break;
    case GL_DEBUG_TYPE_MARKER:
      type_str = "Marker";
      break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
      type_str = "Push Group";
      break;
    case GL_DEBUG_TYPE_POP_GROUP:
      type_str = "Pop Group";
      break;
    case GL_DEBUG_TYPE_OTHER:
      type_str = "Other";
      break;
    default:
      type_str = "Unknown";
      break;
  }

  const char* severity_str;
  switch (severity)
  {
    case GL_DEBUG_SEVERITY_HIGH:
      severity_str = "HIGH";
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      severity_str = "MEDIUM";
      break;
    case GL_DEBUG_SEVERITY_LOW:
      severity_str = "LOW";
      break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      severity_str = "NOTIFICATION";
      break;
    default:
      severity_str = "UNKNOWN";
      break;
  }

  LOG(
    "[OpenGL Debug] Source: {} | Type: {} | Severity: {} | ID: {}\n    Message: {}",
    source_str,
    type_str,
    severity_str,
    id,
    message
  );
}
#endif

i32
main()
{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

#ifdef GAME_DEBUG
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_Window* window = SDL_CreateWindow(
    "game",
    dimensions.first,
    dimensions.second,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
  );
  if (!window)
  {
    SDL_Log("Error creating window: %s\n", SDL_GetError());
    return 1;
  }

  SDL3AudioBuffer audio_buffer = {};
  audio_buffer.spec.format = SDL_AUDIO_S16;
  audio_buffer.spec.channels = 2;
  audio_buffer.spec.freq = 48000;

  SDL_AudioStream* audio_stream =
    SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_buffer.spec, 0, 0);
  if (!audio_stream)
  {
    SDL_Log("Error opening audio stream: %s\n", SDL_GetError());
    return 1;
  }

  // TODO(szulf): do i really want just enough audio for one tick?
  audio_buffer.sample_count = (u32) (((audio_buffer.spec.freq * audio_buffer.spec.channels) / TPS));
  audio_buffer.size = audio_buffer.sample_count * sizeof(i16);
  // TODO(szulf): put this into the perm arena??
  audio_buffer.memory = (i16*) SDL_malloc(audio_buffer.size);

  SDL_GLContext glContext = SDL_GL_CreateContext(window);
  if (!glContext)
  {
    SDL_Log("Error creating OpenGL context: %s\n", SDL_GetError());
    return 1;
  }

  setup_ogl_functions();

  glViewport(0, 0, dimensions.first, dimensions.second);

#ifdef GAME_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debug_callback, 0);
#endif

  std::vector<game::InputEvent> input_events{};
  game::Input input{input_events};

  game::Game game{};

  SDL_ResumeAudioStreamDevice(audio_stream);

  SDL_Event e;
  bool running = true;
  u8 update_tick = 0;
  u8 last_tick = 0;
  u64 starter_ms = SDL_GetTicks();
  while (running)
  {
    u64 ms = SDL_GetTicks() - starter_ms;
    u32 current_second_ms = (u32) ms % 1000;
    update_tick = (u8) current_second_ms / MSPT;
    u8 safe_update_tick = update_tick;
    if (last_tick > update_tick)
    {
      safe_update_tick += 20;
    }

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
          dimensions = {e.window.data1, e.window.data2};
          glViewport(0, 0, dimensions.first, dimensions.second);
        }
        break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
          if (e.button.button == 1)
          {
            input.input_events.push_back({game::Key::Lmb});
          }
        }
        break;
        case SDL_EVENT_KEY_DOWN:
        {
          switch (e.key.key)
          {
            case SDLK_SPACE:
            {
              input.input_events.push_back({game::Key::Space});
            }
            break;
          }
        }
        break;
      }
    }

    for (u8 tick = last_tick; tick < safe_update_tick; ++tick)
    {
      // NOTE(szulf): to really get the current tick you have to do tick % 20

      game::SoundBuffer sound_buffer = {};
      sound_buffer.memory = audio_buffer.memory;
      sound_buffer.size = audio_buffer.size;
      sound_buffer.sample_count = audio_buffer.sample_count;
      sound_buffer.samples_per_second = (u32) audio_buffer.spec.freq;

      game.get_sound(sound_buffer);
      if (!SDL_PutAudioStreamData(audio_stream, sound_buffer.memory, (i32) sound_buffer.size))
      {
        SDL_Log("Error putting audio stream data: %s\n", SDL_GetError());
        return 1;
      }
    }

    for (u8 tick = last_tick; tick < safe_update_tick; ++tick)
    {
      game.update(input);
    }
    game.update_frame();

    game.render();
    SDL_GL_SwapWindow(window);

    u64 new_ms = SDL_GetTicks() - starter_ms;
    u64 ms_in_frame = new_ms - ms;
    if (ms_in_frame < MSPF)
    {
      SDL_Delay((u32) (MSPF - ms_in_frame));
    }
    last_tick = update_tick;
  }

  return 0;
}
