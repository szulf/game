#include "game.cpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include "sdl3_game.h"

// TODO(szulf): set better starting dimensions
platform::WindowDimensions dimensions = {640, 480};

namespace platform
{

Result<void*> read_entire_file(mem::Arena& arena, const char* path, usize* bytes_read)
{
  usize read;

  void* file = SDL_LoadFile(path, &read);
  if (file == nullptr)
  {
    return {Error::FileReadingError};
  }

  auto res = arena.alloc(read);
  if (res.has_error)
  {
    return {Error::FileReadingError};
  }

  mem::copy(res.val, file, read + 1);
  SDL_free(file);
  if (bytes_read)
  {
    *bytes_read = read;
  }

  return {res.val};
}

void print(const char* msg)
{
  SDL_Log("%s\n", msg);
}

[[maybe_unused]] u64 get_ms()
{
  return SDL_GetTicks();
}

WindowDimensions get_window_dimensions()
{
  return dimensions;
}

}

// TODO(szulf): implement these myself later and move to math.h
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

// TODO(szulf): hate it here
void write_val(String& buf, f32 val)
{
  auto written = SDL_snprintf(buf.data + buf.len, buf.cap - buf.len, "%f", val);
  buf.len += static_cast<usize>(written);
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

i32 main()
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

  i32 width = 640;
  i32 height = 480;
  auto* window = SDL_CreateWindow("game", dimensions.width, dimensions.height,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  if (window == nullptr)
  {
    SDL_Log("Error creating window: %s\n", SDL_GetError());
    return 1;
  }

  sdl3::AudioBuffer audio_buffer = {};
  audio_buffer.spec.format   = SDL_AUDIO_S16;
  audio_buffer.spec.channels = 2;
  audio_buffer.spec.freq     = 48000;

  auto* audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                 &audio_buffer.spec, nullptr, nullptr);
  if (!audio_stream)
  {
    SDL_Log("Error opening audio stream: %s\n", SDL_GetError());
    return 1;
  }

  // TODO(szulf): do i really want just enough audio for one tick?
  audio_buffer.sample_count = static_cast<u32>(
    ((audio_buffer.spec.freq * audio_buffer.spec.channels) / TPS));
  audio_buffer.size         = audio_buffer.sample_count * sizeof(i16);
  audio_buffer.memory       = static_cast<i16*>(SDL_malloc(audio_buffer.size));

  auto glContext = SDL_GL_CreateContext(window);
  if (!glContext)
  {
    SDL_Log("Error creating OpenGL context: %s\n", SDL_GetError());
    return 1;
  }

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
  {
    SDL_Log("Error loading glad\n");
    return 1;
  }

  glViewport(0, 0, width, height);

#ifdef GAME_DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debug_callback, nullptr);
#endif

  mem::Arena perm_arena{};
  perm_arena.buffer_size = MEGABYTES(100);
  perm_arena.buffer = SDL_malloc(perm_arena.buffer_size);

  mem::Arena temp_arena{};
  temp_arena.buffer_size = MEGABYTES(100);
  temp_arena.buffer = SDL_malloc(temp_arena.buffer_size);

  game::State state{};

  game::setup(perm_arena, temp_arena, state);

  SDL_Event e;
  bool running = true;

  i8  update_tick = 0;
  i8  last_tick   = 0;
  u32 last_second = 0;
  u64 starter_ms  = SDL_GetTicks();

  SDL_ResumeAudioStreamDevice(audio_stream);

  while (running)
  {
    u64 ms;
    {
      ms = SDL_GetTicks() - starter_ms;
      u32 current_second_ms = ms % 1000;
      u32 current_second = static_cast<u32>(ms / 1000);

      if (current_second != last_second)
      {
        update_tick = 0;
      }

      update_tick = static_cast<i8>(current_second_ms / MSPT);

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
            dimensions = {e.window.data1, e.window.data2};
            glViewport(0, 0, dimensions.width, dimensions.height);
          } break;
        }
      }
    }

    {
      i8 safe_update_tick = update_tick;
      if (last_tick > update_tick)
      {
        safe_update_tick += 20;
      }

      for (i8 tick = last_tick; tick < safe_update_tick; ++tick)
      {
        // NOTE(szulf): to really get the current tick you have to do tick % 20

        game::SoundBuffer sound_buffer = {};
        sound_buffer.memory = audio_buffer.memory;
        sound_buffer.size = audio_buffer.size;
        sound_buffer.sample_count = audio_buffer.sample_count;
        sound_buffer.samples_per_second = static_cast<u32>(audio_buffer.spec.freq);

        game::get_sound(sound_buffer);
        if (!SDL_PutAudioStreamData(audio_stream, sound_buffer.memory,
                                    static_cast<i32>(sound_buffer.size)))
        {
          SDL_Log("Error putting audio stream data: %s\n", SDL_GetError());
          return 1;
        }
      }

      for (i8 tick = last_tick; tick < safe_update_tick; ++tick)
      {
        game::update(state);
      }
    }

    {
      game::render(state);

      SDL_GL_SwapWindow(window);
    }

    {
      u64 new_ms = SDL_GetTicks() - starter_ms;
      u64 ms_in_frame = new_ms - ms;

      if (ms_in_frame < MSPF)
      {
        SDL_Delay(static_cast<u32>(MSPF - ms_in_frame));
      }

      last_tick = update_tick;
    }
  }

  return 0;
}
