#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef RENDERER_OPENGL
#  include "gl_functions.h"
#else
#  error Unknown rendering backend.
#endif

static u32 FPS = 165;
#define MSPF (1000 / FPS)

extern "C"
{
  // TODO(szulf): actually handle errors from read_file and write_file
#define READ_FILE_FN(name) void* name(const char* path, Allocator* allocator, usize* out_size)
  typedef READ_FILE_FN(ReadFileFN);
#define WRITE_FILE_FN(name) void name(const char* path, const String* string)
  typedef WRITE_FILE_FN(WriteFileFN);
#define GET_WIDTH_FN(name) u32 name()
  typedef GET_WIDTH_FN(GetWidthFN);
#define GET_HEIGHT_FN(name) u32 name()
  typedef GET_HEIGHT_FN(GetHeightFN);

  struct PlatformAPI
  {
    ReadFileFN* read_file;
    WriteFileFN* write_file;
    GetWidthFN* get_width;
    GetHeightFN* get_height;
  };

  struct GameMemory
  {
    void* memory;
    usize size;
  };

  struct GameSpec
  {
    const char* name;
    u32 width;
    u32 height;
    usize memory_size;
  };

  struct KeyState
  {
    Key key;
    u32 transition_count;
    bool ended_down;
  };

  struct GameInput
  {
    union
    {
      KeyState states[9];
      struct
      {
        KeyState move_front;
        KeyState move_back;
        KeyState move_left;
        KeyState move_right;
        KeyState interact;

        KeyState camera_move_up;
        KeyState camera_move_down;
        KeyState toggle_camera_mode;
        KeyState toggle_display_bounding_boxes;
      };
    };

    Vec2 mouse_pos;
    Vec2 mouse_relative;
    Vec2 mouse_pos_last;
  };

  enum EventType
  {
    EVENT_TYPE_WINDOW_RESIZE,
  };

  struct WindowResize
  {
    u32 width;
    u32 height;
  };

  struct Event
  {
    EventType type;
    union
    {
      WindowResize window_resize;
    } data;
  };

#define SPEC_FN(name) void name(GameSpec* spec)
  typedef SPEC_FN(SpecFN);
#define APIS_FN(name) void name(RenderingAPI* rendering_api, PlatformAPI* platform_api)
  typedef APIS_FN(APIsFN);
#define INIT_FN(name) void name(GameMemory* memory, GameInput* input)
  typedef INIT_FN(InitFN);
#define POST_RELOAD_FN(name) void name(GameMemory* memory)
  typedef POST_RELOAD_FN(PostReloadFN);
#define UPDATE_FN(name) void name(GameMemory* memory, GameInput* input, float dt)
  typedef UPDATE_FN(UpdateFN);
#define RENDER_FN(name) void name(GameMemory* memory)
  typedef RENDER_FN(RenderFN);
#define EVENT_FN(name) void name(GameMemory* memory, Event* event)
  typedef EVENT_FN(EventFN);

  struct GameAPI
  {
    SpecFN* spec;
    APIsFN* apis;
    InitFN* init;
    PostReloadFN* post_reload;
    UpdateFN* update;
    RenderFN* render;
    EventFN* event;
  };
}

#endif
