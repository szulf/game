#include "platform.h"

static PlatformAPI platform;

#include "image.cpp"

#ifdef RENDERER_OPENGL
#  include "gl_functions.h"
static OpenGLAPI gl;
#  include "gl_renderer.cpp"
#endif

#include "entity.cpp"

// TODO(szulf): collision checking for the light bulb

// TODO(szulf): somehow strip debug mode from release builds

enum GameAction
{
  ACTION_MOVE_UP,
  ACTION_MOVE_DOWN,
  ACTION_MOVE_FRONT,
  ACTION_MOVE_BACK,
  ACTION_MOVE_LEFT,
  ACTION_MOVE_RIGHT,
  ACTION_TOGGLE_DEBUG_MODE,
  ACTION_INTERACT,
};

struct Main
{
  Allocator allocator;

  AssetManager asset_manager;
  u32 shader_map[SHADER_DEFAULT + 1];
  // TODO(szulf): do i really want this here?
  Array<DrawCall> renderer_queue;

  bool debug_mode;

  Camera camera;
  // TODO(szulf): is a map to an array of entities by their type a good idea?
  Array<Entity> entities;
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
  auto& main = *(Main*) memory->memory;

  main.allocator.size = GB(1);
  main.allocator.buffer = (u8*) memory->memory + sizeof(Main);
  main.allocator.type = ALLOCATOR_TYPE_ARENA;

  renderer_init();
  shader_init(main.shader_map);
  shader_map_instance = main.shader_map;
  main.renderer_queue = array_make<DrawCall>(50, main.allocator);
  renderer_queue_instance = &main.renderer_queue;
  main.asset_manager = asset_manager_make(main.allocator);
  asset_manager_instance = &main.asset_manager;

  // TODO(szulf): i dont really like this
  main.entities = array_make<Entity>(30, main.allocator);

  // TODO(szulf): load all entities from a file
  {
    Entity player = {};
    auto player_model = assets_load_model("assets/bean.obj", main.allocator, error);
    ASSERT(error == SUCCESS, "failed to load model");
    player.position = {0.0f, 0.0f, 0.0f};
    player.scale = 0.8f;
    player.has_model = true;
    player.model = player_model;
    player.type = ENTITY_TYPE_PLAYER;
    array_push(main.entities, player);

    auto ground_model = assets_load_model("assets/cube.obj", main.allocator, error);
    ASSERT(error == SUCCESS, "failed to load model");
    for (i32 row = -2; row < 3; ++row)
    {
      for (i32 column = -2; column < 2; ++column)
      {
        Entity ground = {};
        ground.position = {(f32) row, -1.0f, (f32) column};
        ground.scale = 1.0f;
        ground.has_model = true;
        ground.model = ground_model;
        ground.type = ENTITY_TYPE_STATIC_COLLISION;
        array_push(main.entities, ground);
      }
    }

    Entity light_bulb = {};
    auto light_bulb_model = assets_load_model("assets/light_bulb.obj", main.allocator, error);
    ASSERT(error == SUCCESS, "failed to load model");
    light_bulb.position = {-1.0f, 0.0f, 0.0f};
    light_bulb.scale = 1.0f;
    light_bulb.has_model = true;
    light_bulb.model = light_bulb_model;
    light_bulb.type = ENTITY_TYPE_INTERACTABLE;
    light_bulb.interactable_type = INTERACTABLE_TYPE_LIGHT_BULB;
    array_push(main.entities, light_bulb);
  }

  main.camera = {};
  main.camera.pos = {0.0f, 5.0f, 5.0f};
  main.camera.front = {0.0f, 0.0f, -1.0f};
  main.camera.yaw = -90.0f;
  main.camera.pitch = -50.0f;
  main.camera.fov = 45.0f;
  main.camera.near_plane = 0.1f;
  main.camera.far_plane = 1000.0f;
  main.camera.viewport_width = platform.get_width();
  main.camera.viewport_height = platform.get_height();
  camera_update_vectors(main.camera);

  // TODO(szulf): read from keymap file
  input->move_front_key = KEY_W;
  input->move_back_key = KEY_S;
  input->move_left_key = KEY_A;
  input->move_right_key = KEY_D;
  input->toggle_debug_mode_key = KEY_F1;
  input->interact_key = KEY_E;
}

dll_export POST_RELOAD_FN(post_reload)
{
  auto& main = *(Main*) memory->memory;
  shader_map_instance = main.shader_map;
  renderer_queue_instance = &main.renderer_queue;
  asset_manager_instance = &main.asset_manager;
}

dll_export UPDATE_FN(update)
{
  auto& main = *(Main*) memory->memory;

  Entity* player = nullptr;
  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));
  Array<Entity*> ground = array_make<Entity*>(30, scratch_arena.allocator);
  Array<Entity*> interactables = array_make<Entity*>(30, scratch_arena.allocator);
  // TODO(szulf): i dont like that i have to iterate over the whole entities list to find anything
  // but maybe a more complex solution is not worth implementing as of now
  for (usize i = 0; i < main.entities.size; ++i)
  {
    auto& entity = main.entities[i];
    switch (entity.type)
    {
      case ENTITY_TYPE_PLAYER:
      {
        player = &entity;
      }
      break;
      case ENTITY_TYPE_STATIC_COLLISION:
      {
        array_push(ground, &entity);
      }
      break;
      case ENTITY_TYPE_INTERACTABLE:
      {
        array_push(interactables, &entity);
      }
      break;
    }
  }

  if (input->toggle_debug_mode)
  {
    main.debug_mode = !main.debug_mode;
  }

  if (main.debug_mode)
  {
    f32 sensitivity = 1.0f;
    f32 x_offset = input->mouse_relative.x * sensitivity;
    f32 y_offset = input->mouse_relative.y * sensitivity;
    main.camera.yaw += x_offset;
    main.camera.pitch -= y_offset;
    main.camera.pitch = f32_clamp(main.camera.pitch, -89.0f, 89.0f);
    camera_update_vectors(main.camera);

    main.camera.pos += main.camera.front * (-input->move.z * CAMERA_SPEED * dt);
    main.camera.pos += main.camera.right * (input->move.x * CAMERA_SPEED * dt);
  }
  else
  {
    auto new_player_pos = player->position + (input->move * PLAYER_SPEED * dt);
    for (usize i = 0; i < ground.size; ++i)
    {
      auto& g = *ground[i];
      if (g.position == Vec3{f32_round(new_player_pos.x), -1.0f, f32_round(new_player_pos.z)})
      {
        player->position += input->move * PLAYER_SPEED * dt;
        break;
      }
    }
  }

  if (input->interact)
  {
    for (usize i = 0; i < interactables.size; ++i)
    {
      auto& interactable = *interactables[i];
      f32 dist = vec3_len2(player->position - interactable.position);
      if (dist < 1.0f)
      {
        if (interactable.interactable_type == INTERACTABLE_TYPE_LIGHT_BULB)
        {
          print("interacted with light bulb\n");
        }
      }
    }
  }
}

dll_export RENDER_FN(render)
{
  auto& main = *(Main*) memory->memory;

  renderer_clear_screen();

  for (usize i = 0; i < main.entities.size; ++i)
  {
    auto draw_call = draw_call_make(main.entities[i], main.camera);
    renderer_queue_draw_call(draw_call);
  }

  renderer_draw();
}

dll_export EVENT_FN(event)
{
  auto& main = *(Main*) memory->memory;

  switch (event->type)
  {
    case EVENT_TYPE_WINDOW_RESIZE:
    {
      renderer_window_resize(event->data.window_resize.width, event->data.window_resize.height);
      main.camera.viewport_width = event->data.window_resize.width;
      main.camera.viewport_height = event->data.window_resize.height;
    }
    break;
  }
}
