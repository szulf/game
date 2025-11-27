#include "platform.h"

static PlatformAPI platform;

#include "image.cpp"

#ifdef RENDERER_OPENGL
#  include "gl_functions.h"
static OpenGLAPI gl;
#  include "gl_renderer.cpp"
#endif

#include "print.cpp"

struct Main
{
  AssetManager asset_manager;
  Allocator allocator;
  Scene scene;
  u32 shader_map[SHADER_DEFAULT + 1];
};

dll_export SPEC_FN(spec)
{
  spec->name = "game";
  spec->width = 1280;
  spec->height = 720;
  spec->memory_size = GB(2);
}

dll_export APIS_FN(apis)
{
  gl = *gl_api;
  platform = *platform_api;
}

dll_export INIT_FN(init)
{
  auto* main = (Main*) memory->memory;

  renderer_init();
  shader_init(main->shader_map);
  shader_map_instance = main->shader_map;

  main->allocator.size = GB(1);
  main->allocator.buffer = (u8*) memory->memory + sizeof(Main);
  main->allocator.type = ALLOCATOR_TYPE_ARENA;

  main->asset_manager = asset_manager_make(&main->allocator);
  asset_manager_instance = &main->asset_manager;

  Error error = SUCCESS;
  auto renderables = array_make<Renderable>(1, &main->allocator);
  array_push_rvalue(&renderables, {assets_load_model("assets/backpack.obj", &main->allocator, &error), SHADER_DEFAULT});
  ASSERT(error == SUCCESS, "failed to load model");

  Vec3 camera_pos = {0.0f, 0.0f, -5.0f};
  main->scene = {renderables, camera_make(&camera_pos, platform.get_width(), platform.get_height())};
}

dll_export REINIT_FN(reinit)
{
  auto* main = (Main*) memory->memory;
  shader_map_instance = main->shader_map;
  asset_manager_instance = &main->asset_manager;
}

dll_export UPDATE_FN(update)
{
  auto* main = (Main*) memory->memory;

  const f32 add = 1.0f;
  main->scene.camera.yaw += add;
  camera_update_vectors(&main->scene.camera);

  unused(dt);
}

dll_export RENDER_FN(render)
{
  auto* main = (Main*) memory->memory;

  renderer_clear_screen();
  renderer_render(&main->scene);
}

dll_export EVENT_FN(event)
{
  auto* main = (Main*) memory->memory;
  main->scene.camera.viewport_width = event->width;
  main->scene.camera.viewport_height = event->height;
}
