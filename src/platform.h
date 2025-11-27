#ifndef PLATFORM_H
#define PLATFORM_H

#include "base/base.cpp"

#define TPS 20
#define MSPT (1000 / TPS)

extern "C"
{
  struct OpenGLAPI;

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

  // TODO(szulf): make this the tagged union
  struct Event
  {
    u32 width;
    u32 height;
  };

#define SPEC_FN(name) void name(GameSpec* spec)
  typedef SPEC_FN(SpecFN);
#define APIS_FN(name) void name(OpenGLAPI* gl_api, PlatformAPI* platform_api)
  typedef APIS_FN(APIsFN);
#define INIT_FN(name) void name(GameMemory* memory)
  typedef INIT_FN(InitFN);
#define POST_RELOAD_FN(name) void name(GameMemory* memory)
  typedef POST_RELOAD_FN(PostReloadFN);
#define UPDATE_FN(name) void name(GameMemory* memory, float dt)
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
