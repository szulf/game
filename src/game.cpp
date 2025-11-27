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

enum Action
{
  ACTION_MOVE_FRONT,
  ACTION_MOVE_BACK,
  ACTION_MOVE_LEFT,
  ACTION_MOVE_RIGHT,
};

struct Main
{
  Allocator allocator;

  AssetManager asset_manager;
  u32 shader_map[SHADER_DEFAULT + 1];
  // TODO(szulf): do i really want this here?
  Array<DrawCall> renderer_queue;

  // TODO(szulf): this is just wasteful, it should only contain ACTION_COUNT,
  // but i dont know how to easily index it if it does
  Action key_map[KEY_COUNT];
  Camera camera;
  Entity player;
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

  auto player_model = assets_load_model("assets/backpack.obj", &main->allocator, &error);
  ASSERT(error == SUCCESS, "failed to load model");
  main->player.position = {0.0f, 0.0f, 0.0f};
  main->player.scale = 1.0f;
  main->player.has_model = true;
  main->player.model = player_model;
  main->player.type = ENTITY_TYPE_PLAYER;

  Vec3 camera_pos = {0.0f, 0.0f, -5.0f};
  main->camera = camera_make(&camera_pos, platform.get_width(), platform.get_height());

  main->key_map[KEY_W] = ACTION_MOVE_FRONT;
  main->key_map[KEY_S] = ACTION_MOVE_BACK;
  main->key_map[KEY_A] = ACTION_MOVE_LEFT;
  main->key_map[KEY_D] = ACTION_MOVE_RIGHT;
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

  unused(main);
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

  auto player_draw_call = draw_call_make(&main->player, &main->camera);
  renderer_queue_draw_call(&player_draw_call);
  renderer_draw();
}

dll_export EVENT_FN(event)
{
  auto* main = (Main*) memory->memory;

  switch (event->type)
  {
    case EVENT_TYPE_WINDOW_RESIZE:
    {
      main->camera.viewport_width = event->data.window_resize.width;
      main->camera.viewport_height = event->data.window_resize.height;
    }
    break;
    case EVENT_TYPE_KEYDOWN:
    {
      auto action = main->key_map[event->data.keydown.key];
      switch (action)
      {
        case ACTION_MOVE_FRONT:
        {
          main->player.position.z -= 1.0f;
        }
        break;
        case ACTION_MOVE_BACK:
        {
          main->player.position.z += 1.0f;
        }
        break;
        case ACTION_MOVE_LEFT:
        {
          main->player.position.x -= 1.0f;
        }
        break;
        case ACTION_MOVE_RIGHT:
        {
          main->player.position.x += 1.0f;
        }
        break;
      }
    }
    break;
  }
}
