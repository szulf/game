#include "game.c"

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include "sdl3_game.h"

// TODO(szulf): set better starting dimensions
WindowDimensions dimensions = {640, 480};

static void*
platform_read_entire_file(Arena* arena, const char* path, Error* err)
{
  return platform_read_entire_file_bytes_read(arena, path, 0, err);
}

static void*
platform_read_entire_file_bytes_read(Arena* arena, const char* path, usize* bytes_read, Error* err)
{
  Error error = ERROR_SUCCESS;

  usize read;
  void* file = SDL_LoadFile(path, &read);
  if (file == 0)
  {
    ASSERT(false, "couldnt read file");
    *err = ERROR_FILE_READING;
    return 0;
  }

  void* file_data = arena_alloc(arena, read, &error);
  if (error != ERROR_SUCCESS)
  {
    ASSERT(false, "couldnt allocate memory for file");
    *err = error;
    return 0;
  }

  mem_copy(file_data, file, read);
  SDL_free(file);
  if (bytes_read)
  {
    *bytes_read = read;
  }

  *err = ERROR_SUCCESS;
  return file_data;
}

static void
platform_print(const char* msg, ...)
{
  va_list ap;
  SDL_Log(msg, ap);
}

static u64
get_ms()
{
  return SDL_GetTicks();
}

static WindowDimensions
get_window_dimensions()
{
  return dimensions;
}

// TODO(szulf): implement these myself later and move to math.c
f32 sin(f32 rad)
{
  return SDL_sinf(rad);
}

f32 cos(f32 rad)
{
  return SDL_cosf(rad);
}

f32 sqrt(f32 val)
{
  return SDL_sqrtf(val);
}

f32 mod(f32 x, f32 y)
{
  ASSERT(y != 0, "denominator cannot be 0");
  return SDL_fmodf(x, y);
}

f32 acos(f32 val)
{
  return SDL_acosf(val);
}

f32 tan(f32 val)
{
  return SDL_tanf(val);
}

#ifdef GAME_DEBUG
// TODO(szulf): change this to use the LOG macro
static void APIENTRY debug_callback(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* user)
{
  (void) length;
  (void) user;

  // Ignore non-significant notification messages
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
      return;

  const char* source_str;
  switch (source) {
      case GL_DEBUG_SOURCE_API:             source_str = "API"; break;
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_str = "Window System"; break;
      case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "Shader Compiler"; break;
      case GL_DEBUG_SOURCE_THIRD_PARTY:     source_str = "Third Party"; break;
      case GL_DEBUG_SOURCE_APPLICATION:     source_str = "Application"; break;
      case GL_DEBUG_SOURCE_OTHER:           source_str = "Other"; break;
      default:                              source_str = "Unknown"; break;
  }

  const char* type_str;
  switch (type) {
      case GL_DEBUG_TYPE_ERROR:               type_str = "Error"; break;
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "Deprecated Behaviour"; break;
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_str = "Undefined Behaviour"; break;
      case GL_DEBUG_TYPE_PORTABILITY:         type_str = "Portability"; break;
      case GL_DEBUG_TYPE_PERFORMANCE:         type_str = "Performance"; break;
      case GL_DEBUG_TYPE_MARKER:              type_str = "Marker"; break;
      case GL_DEBUG_TYPE_PUSH_GROUP:          type_str = "Push Group"; break;
      case GL_DEBUG_TYPE_POP_GROUP:           type_str = "Pop Group"; break;
      case GL_DEBUG_TYPE_OTHER:               type_str = "Other"; break;
      default:                                type_str = "Unknown"; break;
  }

  const char* severity_str;
  switch (severity) {
      case GL_DEBUG_SEVERITY_HIGH:         severity_str = "HIGH"; break;
      case GL_DEBUG_SEVERITY_MEDIUM:       severity_str = "MEDIUM"; break;
      case GL_DEBUG_SEVERITY_LOW:          severity_str = "LOW"; break;
      case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "NOTIFICATION"; break;
      default:                             severity_str = "UNKNOWN"; break;
  }

  SDL_Log("[OpenGL Debug] Source: %s | Type: %s | Severity: %s | ID: %u\n    Message: %s",
          source_str, type_str, severity_str, id, message);
}
#endif

s32
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

  SDL_Window* window = SDL_CreateWindow("game", dimensions.width, dimensions.height,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  if (!window)
  {
    SDL_Log("Error creating window: %s\n", SDL_GetError());
    return 1;
  }

  SDL3AudioBuffer audio_buffer = {};
  audio_buffer.spec.format   = SDL_AUDIO_S16;
  audio_buffer.spec.channels = 2;
  audio_buffer.spec.freq     = 48000;

  SDL_AudioStream* audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                            &audio_buffer.spec, 0, 0);
  if (!audio_stream)
  {
    SDL_Log("Error opening audio stream: %s\n", SDL_GetError());
    return 1;
  }

  // TODO(szulf): do i really want just enough audio for one tick?
  audio_buffer.sample_count = (u32) (((audio_buffer.spec.freq * audio_buffer.spec.channels) / TPS));
  audio_buffer.size         = audio_buffer.sample_count * sizeof(s16);
  // TODO(szulf): put this into the perm arena??
  audio_buffer.memory       = (s16*) SDL_malloc(audio_buffer.size);

  SDL_GLContext glContext = SDL_GL_CreateContext(window);
  if (!glContext)
  {
    SDL_Log("Error creating OpenGL context: %s\n", SDL_GetError());
    return 1;
  }

  if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
  {
    SDL_Log("Error loading glad\n");
    return 1;
  }

  glViewport(0, 0, dimensions.width, dimensions.height);

#ifdef GAME_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debug_callback, 0);
#endif

  Arena perm_arena = {};
  perm_arena.buffer_size = MEGABYTES(100);
  perm_arena.buffer = SDL_malloc(perm_arena.buffer_size);

  Arena temp_arena = {};
  temp_arena.buffer_size = MEGABYTES(100);
  temp_arena.buffer = SDL_malloc(temp_arena.buffer_size);

  GameState state = {};

  InputEventArray input_events = {};
  {
    Error error = ERROR_SUCCESS;
    ARRAY_INIT(&input_events, &perm_arena, 100, &error);
    ASSERT(error == ERROR_SUCCESS, "couldnt init inputs array");
  }
  GameInput input = {input_events};

  game_setup(&perm_arena, &temp_arena, &state);

  SDL_Event e;
  bool32 running = true;

  s8  update_tick = 0;
  s8  last_tick   = 0;
  u32 last_second = 0;
  u64 starter_ms  = SDL_GetTicks();

  SDL_ResumeAudioStreamDevice(audio_stream);

  while (running)
  {
    u64 ms;
    {
      ms = SDL_GetTicks() - starter_ms;
      u32 current_second_ms = (u32) ms % 1000;
      u32 current_second = (u32) (ms / 1000);

      if (current_second != last_second)
      {
        update_tick = 0;
      }

      update_tick = (s8) current_second_ms / MSPT;

      last_second = current_second;
    }

    {
      while (SDL_PollEvent(&e))
      {
        switch (e.type)
        {
          case SDL_EVENT_QUIT:
          {
            running = false;
          } break;
          case SDL_EVENT_WINDOW_RESIZED:
          {
            dimensions = (WindowDimensions) {e.window.data1, e.window.data2};
            glViewport(0, 0, dimensions.width, dimensions.height);
          } break;
          case SDL_EVENT_MOUSE_BUTTON_DOWN:
          {
            if (e.button.button == 1)
            {
              ARRAY_PUSH(&input.input_events, ((InputEvent) {.key = KEY_LMB}));
            }
          } break;
          case SDL_EVENT_KEY_DOWN:
          {
            switch (e.key.key)
            {
              case SDLK_SPACE:
              {
                ARRAY_PUSH(&input.input_events, ((InputEvent) {.key = KEY_SPACE}));
              } break;
            }
          } break;
        }
      }
    }

    {
      s8 safe_update_tick = update_tick;
      if (last_tick > update_tick)
      {
        safe_update_tick += 20;
      }

      for (s8 tick = last_tick; tick < safe_update_tick; ++tick)
      {
        // NOTE(szulf): to really get the current tick you have to do tick % 20

        GameSoundBuffer sound_buffer = {};
        sound_buffer.memory = audio_buffer.memory;
        sound_buffer.size = audio_buffer.size;
        sound_buffer.sample_count = audio_buffer.sample_count;
        sound_buffer.samples_per_second = (u32) audio_buffer.spec.freq;

        game_get_sound(&sound_buffer);
        if (!SDL_PutAudioStreamData(audio_stream, sound_buffer.memory, (s32) sound_buffer.size))
        {
          SDL_Log("Error putting audio stream data: %s\n", SDL_GetError());
          return 1;
        }
      }

      for (s8 tick = last_tick; tick < safe_update_tick; ++tick)
      {
        game_update(&state, &input);
      }
    }

    {
      game_render(&state);

      SDL_GL_SwapWindow(window);
    }

    {
      u64 new_ms = SDL_GetTicks() - starter_ms;
      u64 ms_in_frame = new_ms - ms;

      if (ms_in_frame < MSPF)
      {
        SDL_Delay((u32) (MSPF - ms_in_frame));
      }

      last_tick = update_tick;
    }
  }

  return 0;
}
