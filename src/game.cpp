#include "platform.h"

static PlatformAPI platform;

#include "image.cpp"

#ifdef RENDERER_OPENGL
#  include "gl_functions.h"
static OpenGLAPI gl;
#  include "gl_renderer.cpp"
#endif

enum EntityType
{
  ENTITY_TYPE_PLAYER,
};

struct Entity
{
  EntityType type;

  // TODO(szulf): quaternions for orientation? or just euler angles?
  Vec3 position;
  f32 scale;

  b8 has_model;
  ModelHandle model;
};

struct Main
{
  Allocator allocator;

  AssetManager asset_manager;
  u32 shader_map[SHADER_DEFAULT + 1];
  // TODO(szulf): do i really want this here?
  Array<DrawCall> renderer_queue;

  Camera camera;
  Entity entity;
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
  Error error = SUCCESS;
  auto* main = (Main*) memory->memory;

  main->allocator.size = GB(1);
  main->allocator.buffer = (u8*) memory->memory + sizeof(Main);
  main->allocator.type = ALLOCATOR_TYPE_ARENA;

  renderer_init();
  shader_init(main->shader_map);
  shader_map_instance = main->shader_map;
  main->renderer_queue = array_make<DrawCall>(50, &main->allocator);
  renderer_queue_instance = &main->renderer_queue;
  main->asset_manager = asset_manager_make(&main->allocator);
  asset_manager_instance = &main->asset_manager;

  auto model_handle = assets_load_model("assets/backpack.obj", &main->allocator, &error);
  ASSERT(error == SUCCESS, "failed to load model");
  main->entity.position = {0.0f, 0.0f, 0.0f};
  main->entity.scale = 1.0f;
  main->entity.has_model = true;
  main->entity.model = model_handle;
  main->entity.type = ENTITY_TYPE_PLAYER;

  Vec3 camera_pos = {0.0f, 0.0f, -5.0f};
  main->camera = camera_make(&camera_pos, platform.get_width(), platform.get_height());
}

dll_export POST_RELOAD_FN(post_reload)
{
  auto* main = (Main*) memory->memory;
  shader_map_instance = main->shader_map;
  renderer_queue_instance = &main->renderer_queue;
  asset_manager_instance = &main->asset_manager;
}

dll_export UPDATE_FN(update)
{
  auto* main = (Main*) memory->memory;

  const f32 add = 1.0f;
  main->camera.yaw += add;
  camera_update_vectors(&main->camera);

  unused(dt);
}

DrawCall draw_call_make(const Entity* entity, const Camera* camera)
{
  DrawCall out = {};
  if (entity->has_model)
  {
    out.model_handle = entity->model;
  }
  out.model = mat4_make();
  // TODO(szulf): order of operations matter! is this the correct one?
  mat4_scale(&out.model, entity->scale);
  mat4_translate(&out.model, &entity->position);
  out.view = camera_look_at(camera);
  out.projection = camera_projection(camera);
  return out;
}

dll_export RENDER_FN(render)
{
  auto* main = (Main*) memory->memory;

  renderer_clear_screen();

  auto draw_call = draw_call_make(&main->entity, &main->camera);
  renderer_queue_draw_call(&draw_call);
  renderer_draw();
}

dll_export EVENT_FN(event)
{
  auto* main = (Main*) memory->memory;
  main->camera.viewport_width = event->width;
  main->camera.viewport_height = event->height;
}
