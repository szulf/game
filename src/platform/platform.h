#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef RENDERER_OPENGL
#  include "gl_functions.h"
#else
#  error Unknown rendering backend.
#endif

#define TPS 60
#define MSPT (1000 / TPS)

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

  struct GameKeymap
  {
    Key move_front;
    Key move_back;
    Key move_left;
    Key move_right;

    Key interact;

    // NOTE(szulf): debug keybinds
    Key toggle_camera_mode;
    Key toggle_display_bounding_boxes;
  };

  // NOTE(szulf): game input needs to know what keys to check, since there are rebindable keys
  struct GameInput
  {
    GameKeymap key_map;

    Vec3 move;

    bool interact;

    Vec2 mouse_pos;
    Vec2 mouse_relative;
    Vec2 mouse_pos_last;

    bool toggle_camera_mode;
    bool toggle_display_bounding_boxes;
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
#define INIT_FN(name) void name(GameMemory* memory, GameKeymap* keymap)
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
