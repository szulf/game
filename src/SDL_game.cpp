#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <math.h>

#include "glad/src/glad.c"

#include "SDL_game.h"

template <typename T>
T min(T a, T b)
{
  if (a > b)
  {
    return b;
  }

  return a;
}

template <typename T>
T max(T a, T b)
{
  if (a > b)
  {
    return a;
  }

  return b;
}

i32 main()
{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

#if GAME_INTERNAL
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
#else
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  auto *window = SDL_CreateWindow("game", 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  if (window == nullptr)
  {
    fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
    return 1;
  }

  auto glContext = SDL_GL_CreateContext(window);
  if (glContext == nullptr)
  {
    fprintf(stderr, "Error creating OpenGL context: %s\n", SDL_GetError());
    return 1;
  }

  auto audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
  if (!audio_device)
  {
    fprintf(stderr, "Error opening audio device: %s\n", SDL_GetError());
    return 1;
  }

  SDL3AudioBuffer audio_buffer = {};
  audio_buffer.spec.format   = SDL_AUDIO_S16;
  audio_buffer.spec.channels = 2;
  audio_buffer.spec.freq     = 48000;
  audio_buffer.sample_count  = (u32) (audio_buffer.spec.freq * audio_buffer.spec.channels);
  audio_buffer.size          = audio_buffer.sample_count * (i32) sizeof(i16);

  auto audio_stream = SDL_CreateAudioStream(&audio_buffer.spec, &audio_buffer.spec);
  if (!audio_stream)
  {
    fprintf(stderr, "Error creating audio stream: %s\n", SDL_GetError());
    return 1;
  }

  if (!SDL_BindAudioStream(audio_device, audio_stream))
  {
    fprintf(stderr, "Error binding audio stream: %s\n", SDL_GetError());
    return 1;
  }

  audio_buffer.buffer = SDL_malloc((usize) audio_buffer.size);

  SDL3AudioBuffer sound_buffer = {};
  if (!SDL_LoadWAV("./assets/sound.wav", &sound_buffer.spec, (u8**) &sound_buffer.buffer,
                   &sound_buffer.size))
  {
    fprintf(stderr, "Error loading wav sound file\n");
    return 1;
  }

  // NOTE(szulf): here i ahead of time know that sound.wav is encoded with i16s
  sound_buffer.sample_count = sound_buffer.size / sizeof(i16);

  for (u32 i = 0; i < audio_buffer.sample_count; i += 2)
  {
    f32 sample_index = (f32) i / 2.0f;
    f32 t = sample_index / (f32) audio_buffer.spec.freq;
    f32 freq = 440.0f;
    f32 amplitude = 0.25f;

    i16 sine_value = (i16) (sinf(2.0f * PI32 * t * freq) * I16_MAX * amplitude);

    i16 *audio_left = ((i16*) audio_buffer.buffer) + i;
    i16 *audio_right = ((i16*) audio_buffer.buffer) + i + 1;

    i16 *sound_left = ((i16*) sound_buffer.buffer) + i;
    i16 *sound_right = ((i16*) sound_buffer.buffer) + i + 1;

    *audio_left = (i16) max(min(sine_value + *sound_left, I16_MAX), I16_MIN);
    *audio_right = (i16) max(min(sine_value + *sound_right, I16_MAX), I16_MIN);
  }

  if (!SDL_PutAudioStreamData(audio_stream, audio_buffer.buffer, (i32) audio_buffer.size))
  {
    fprintf(stderr, "Error put data in audio stream: %s\n", SDL_GetError());
    return 1;
  }

  if (!SDL_PutAudioStreamData(audio_stream, (u8*) sound_buffer.buffer + audio_buffer.size,
                              (i32) (sound_buffer.size - audio_buffer.size)))
  {
    fprintf(stderr, "Error put data in audio stream: %s\n", SDL_GetError());
    return 1;
  }

  if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress))
  {
    fprintf(stderr, "Error loading glad\n");
    return 1;
  }

  float vertices[] = {
     0.0f,  0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
  };

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);

  const char *v_shader_src = (char*) SDL_LoadFile("src/shader.vert", nullptr);
  if (v_shader_src == nullptr)
  {
    fprintf(stderr, "Error reading vertex shader file: %s\n", SDL_GetError());
    return 1;
  }

  GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(v_shader, 1, &v_shader_src, nullptr);
  glCompileShader(v_shader);
  SDL_free((void*) v_shader_src);

  GLint v_compiled;
  glGetShaderiv(v_shader, GL_COMPILE_STATUS, &v_compiled);
  if (v_compiled != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetShaderInfoLog(v_shader, 1024, &log_length, message);
    fprintf(stderr, "Error compiling vertex shader:\n%s\n", message);
    return 1;
  }

  const char *f_shader_src = (char*) SDL_LoadFile("src/shader.frag", nullptr);
  if (f_shader_src == nullptr)
  {
    fprintf(stderr, "Error reading fragment shader file: %s\n", SDL_GetError());
    return 1;
  }

  GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(f_shader, 1, &f_shader_src, nullptr);
  glCompileShader(f_shader);
  SDL_free((void*) f_shader_src);

  GLint f_compiled;
  glGetShaderiv(f_shader, GL_COMPILE_STATUS, &f_compiled);
  if (f_compiled != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetShaderInfoLog(f_shader, 1024, &log_length, message);
    fprintf(stderr, "Error compiling fragment shader:\n%s\n", message);
    return 1;
  }

  GLuint program = glCreateProgram();
  glAttachShader(program, v_shader);
  glAttachShader(program, f_shader);
  glLinkProgram(program);

  GLint program_linked;
  glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
  if (program_linked != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetProgramInfoLog(program, 1024, &log_length, message);
    fprintf(stderr, "Error compiling fragment shader:\n%s\n", message);
    return 1;
  }

  glUseProgram(program);

  SDL_Event e;
  bool running = true;

  u8 update_tick = 0;
  u32 last_second = 0;

  // TODO(szulf): write audio currently only for 1 frame < 1 tick
  // and just write enough for 1 tick of audio
  while (running)
  {
    // NOTE(szulf): gathering input
    while (SDL_PollEvent(&e))
    {
      switch (e.type)
      {
        case SDL_EVENT_QUIT:
        {
          running = false;
        } break;

      }
    }

    // NOTE(szulf): current game tick computation
    u64 ticks = SDL_GetTicks();
    u32 current_second_ticks = ticks % 1000;
    u32 current_second = (u32) ticks / 1000;

    if (current_second != last_second)
    {
      update_tick = 0;
    }

    if (current_second_ticks > update_tick * 50)
    {
      ++update_tick;
    }

    last_second = current_second;

    // printf("%hhu\n", update_tick);

    // NOTE(szulf): rendering
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(window);
  }

  return 0;
}
