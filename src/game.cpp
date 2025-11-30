#include "platform.h"

static PlatformAPI platform;

#include "image.cpp"

// TODO(szulf): should a check like this happen in the files themselves or here?
#ifdef RENDERER_OPENGL
#  include "gl_functions.h"
static OpenGLAPI gl;
#  include "gl_renderer.cpp"
#endif

#include "entity.cpp"

// TODO(szulf): would be nice to draw interaction radius on interactables in debug mode

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
  main.renderer_queue = array_make<DrawCall>(ARRAY_TYPE_DYNAMIC, 50, main.allocator);
  renderer_queue_instance = &main.renderer_queue;
  main.asset_manager = asset_manager_make(main.allocator);
  asset_manager_instance = &main.asset_manager;

  main.entities = array_make<Entity>(ARRAY_TYPE_DYNAMIC, 30, main.allocator);

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
    player.bounding_box_width = 0.8f;
    player.bounding_box_depth = 0.8f;
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
    // TODO(szulf): hardcoded for now, can i actually calculate this somehow?
    light_bulb.bounding_box_width = 0.5f;
    light_bulb.bounding_box_depth = 0.5f;
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

  // TODO(szulf): i hate this being here
  // NOTE(szulf): creation of the bounding box model
  Material material = {};
  material.shader = SHADER_GREEN;
  auto material_handle = assets_set_material(material);
  Mesh mesh = mesh_make(
    array_from(bounding_box_vertices, array_size(bounding_box_vertices)),
    array_from(bounding_box_indices, array_size(bounding_box_indices)),
    material_handle
  );
  auto mesh_handle = assets_set_mesh(mesh);
  Model model = {};
  model.matrix = mat4_make();
  model.meshes = array_make<MeshHandle>(ARRAY_TYPE_STATIC, 1, main.allocator);
  array_push(model.meshes, mesh_handle);
  g_bounding_box_model = assets_set_model(model);
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
  Array<Entity*> ground = array_make<Entity*>(ARRAY_TYPE_DYNAMIC, 30, scratch_arena.allocator);
  Array<Entity*> interactables = array_make<Entity*>(ARRAY_TYPE_DYNAMIC, 30, scratch_arena.allocator);
  // TODO(szulf): i dont like that i have to iterate over the whole entities list to find anything
  // but maybe a more complex solution is not worth implementing as of now
  // could probably just cache this somehow, and check if new entities have not been added
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
    f32 x_offset = input->mouse_relative.x * CAMERA_SENSITIVITY;
    f32 y_offset = input->mouse_relative.y * CAMERA_SENSITIVITY;
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
    bool can_move = false;
    for (usize i = 0; i < ground.size; ++i)
    {
      auto& g = *ground[i];
      if (g.position == Vec3{f32_round(new_player_pos.x), -1.0f, f32_round(new_player_pos.z)})
      {
        can_move = true;
        break;
      }
    }
    if (can_move)
    {
      for (usize i = 0; i < interactables.size; ++i)
      {
        auto& interactable = *interactables[i];
        Entity test = *player;
        test.position = new_player_pos;
        if (collides(test, interactable))
        {
          can_move = false;
        }
      }
    }
    if (can_move)
    {
      player->position = new_player_pos;
    }
  }

  if (input->interact)
  {
    for (usize i = 0; i < interactables.size; ++i)
    {
      auto& interactable = *interactables[i];
      // TODO(szulf): also check for the orientation of the player?
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
    auto& entity = main.entities[i];
    if (main.debug_mode)
    {
      auto bounding_box_call = draw_call_entity_bounding_box(entity, main.camera);
      renderer_queue_draw_call(bounding_box_call);
    }
    auto draw_call = draw_call_entity(entity, main.camera);
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
