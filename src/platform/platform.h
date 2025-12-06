#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef RENDERER_OPENGL
#  include "gl_functions.h"
#else
#  error Unknown rendering backend.
#endif

#define TPS 20
#define MSPT (1000 / TPS)

extern "C"
{
  typedef void* (*ReadFileFN)(const char* path, Allocator* allocator, usize* out_size);
  typedef u32 (*GetWidthFN)();
  typedef u32 (*GetHeightFN)();

  struct PlatformAPI
  {
    ReadFileFN read_file;
    GetWidthFN get_width;
    GetHeightFN get_height;
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

  struct GameKeyMap
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
    GameKeyMap key_map;

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
#define INIT_FN(name) void name(GameMemory* memory, GameKeyMap* key_map)
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
