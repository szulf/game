#include "base/base.cpp"
#include "platform/platform.h"

static PlatformAPI platform;
static RenderingAPI rendering;

#include "image.cpp"

#include "assets/assets.cpp"
#include "renderer/renderer.cpp"

#include "camera.cpp"
#include "entity.cpp"

struct Main
{
  Allocator allocator;

  AssetManager asset_manager;
  Array<DrawCall> renderer_queue;

  bool camera_mode;
  bool display_bounding_boxes;

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
  rendering = *rendering_api;
  platform = *platform_api;
}

dll_export INIT_FN(init)
{
  Error error = SUCCESS;
  auto& main = *(Main*) memory->memory;

  main.allocator.size = GB(1);
  // TODO(szulf): what about alignment here?
  main.allocator.buffer = (u8*) memory->memory + sizeof(Main);
  main.allocator.type = ALLOCATOR_TYPE_ARENA;

  renderer_init();
  main.renderer_queue = array_make<DrawCall>(ARRAY_TYPE_DYNAMIC, 50, main.allocator);
  main.asset_manager = asset_manager_make(main.allocator);
  asset_manager_instance = &main.asset_manager;

  {
    auto shader = assets_load_shader("shaders/shader.vert", "shaders/green.frag", error);
    ASSERT(error == SUCCESS && shader == SHADER_GREEN, "failed to initalize shader");

    shader = assets_load_shader("shaders/shader.vert", "shaders/yellow.frag", error);
    ASSERT(error == SUCCESS && shader == SHADER_YELLOW, "failed to initalize shader");

    shader = assets_load_shader("shaders/shader.vert", "shaders/shader.frag", error);
    ASSERT(error == SUCCESS && shader == SHADER_DEFAULT, "failed to initalize shader");
  }

  static_model_init(
    STATIC_MODEL_BOUNDING_BOX,
    SHADER_GREEN,
    array_from(bounding_box_vertices, array_size(bounding_box_vertices)),
    array_from(bounding_box_indices, array_size(bounding_box_indices)),
    main.allocator
  );
  static_model_init(
    STATIC_MODEL_RING,
    SHADER_YELLOW,
    array_from(ring_vertices, array_size(ring_vertices)),
    array_from(ring_indices, array_size(ring_indices)),
    main.allocator
  );

  main.entities = array_make<Entity>(ARRAY_TYPE_DYNAMIC, 30, main.allocator);

  // TODO(szulf): load all entities from a file
  {
    Entity player = {};
    auto player_model = assets_load_model("assets/bean.obj", main.allocator, error);
    ASSERT(error == SUCCESS, "failed to load model");
    player.position = {0.0f, 0.0f, 0.0f};
    player.scale = 1.0f;
    player.has_model = true;
    player.model = player_model;
    player.type = ENTITY_TYPE_PLAYER;
    player.bounding_box = bounding_box_from_model(player_model);
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
    light_bulb.bounding_box = bounding_box_from_model(light_bulb_model);
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

  // TODO(szulf): read from file
  key_map->move_front = KEY_W;
  key_map->move_back = KEY_S;
  key_map->move_left = KEY_A;
  key_map->move_right = KEY_D;
  key_map->toggle_camera_mode = KEY_F1;
  key_map->toggle_display_bounding_boxes = KEY_F2;
  key_map->interact = KEY_E;
}

dll_export POST_RELOAD_FN(post_reload)
{
  auto& main = *(Main*) memory->memory;
  asset_manager_instance = &main.asset_manager;
}

dll_export UPDATE_FN(update)
{
  auto& main = *(Main*) memory->memory;

  Entity* player = nullptr;
  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));
  Array<Entity*> ground = array_make<Entity*>(ARRAY_TYPE_DYNAMIC, 30, scratch_arena.allocator);
  Array<Entity*> interactables =
    array_make<Entity*>(ARRAY_TYPE_DYNAMIC, 30, scratch_arena.allocator);
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

  if (input->toggle_camera_mode)
  {
    main.camera_mode = !main.camera_mode;
  }
  if (input->toggle_display_bounding_boxes)
  {
    main.display_bounding_boxes = !main.display_bounding_boxes;
  }

  if (main.camera_mode)
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
      if (dist < interactable_info[interactable.interactable_type].radius2)
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
    if (!entity.has_model)
    {
      continue;
    }
    if (main.display_bounding_boxes && entity.type != ENTITY_TYPE_STATIC_COLLISION)
    {
      auto bounding_box_call = draw_call_entity_bounding_box(entity, main.camera);
      renderer_queue_draw_call(main.renderer_queue, bounding_box_call);
      if (entity.type == ENTITY_TYPE_INTERACTABLE)
      {
        auto radius_call = draw_call_entity_interactable_radius(entity, main.camera);
        renderer_queue_draw_call(main.renderer_queue, radius_call);
      }
    }
    auto draw_call = draw_call_entity(entity, main.camera);
    renderer_queue_draw_call(main.renderer_queue, draw_call);
  }

  renderer_draw(main.renderer_queue);
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
