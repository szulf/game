#include "base/base.cpp"
#include "platform/platform.h"

static PlatformAPI platform;
static RenderingAPI rendering;

#include "image.cpp"

#include "assets/assets.cpp"
#include "renderer/renderer.cpp"

#include "camera.cpp"
#include "entity.cpp"

#include "data/data.cpp"

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
  main.allocator.buffer = (u8*) memory->memory + sizeof(Main);
  main.allocator.type = ALLOCATOR_TYPE_ARENA;

  main.asset_manager = asset_manager_make(main.allocator);
  asset_manager_instance = &main.asset_manager;
  main.renderer_queue = array_make<DrawCall>(ARRAY_TYPE_DYNAMIC, 50, main.allocator);
  renderer_init(main.allocator, error);
  ASSERT(error == SUCCESS, "couldnt initialize renderer");

  main.entities = scene_from_file("data/main.gscn", main.allocator, error);
  ASSERT(error == SUCCESS, "couldnt load scene");

  *input = keymap_from_file("data/keymap.gkey", error);
  ASSERT(error == SUCCESS, "couldnt read keymap file");

  main.camera = {};
  main.camera.pos = {0.0f, 8.0f, 4.0f};
  main.camera.yaw = -90.0f;
  main.camera.pitch = -60.0f;
  main.camera.fov = 45.0f;
  main.camera.near_plane = 0.1f;
  main.camera.far_plane = 1000.0f;
  main.camera.viewport_width = platform.get_width();
  main.camera.viewport_height = platform.get_height();
  camera_update_vectors(main.camera);
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
  // TODO(szulf): basically acts as a bad frame arena, fix that
  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));
  Array<Entity*> collidables = array_make<Entity*>(ARRAY_TYPE_DYNAMIC, 30, scratch_arena.allocator);
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
        array_push(collidables, &entity);
      }
      break;
      case ENTITY_TYPE_INTERACTABLE:
      {
        array_push(collidables, &entity);
        array_push(interactables, &entity);
      }
      break;
    }
  }

  if (input->toggle_camera_mode.ended_down && input->toggle_camera_mode.transition_count != 0)
  {
    main.camera_mode = !main.camera_mode;
  }
  if (input->toggle_display_bounding_boxes.ended_down &&
      input->toggle_display_bounding_boxes.transition_count != 0)
  {
    main.display_bounding_boxes = !main.display_bounding_boxes;
  }

  Vec3 acceleration = {};
  if (input->move_front.ended_down)
  {
    acceleration.z += -1.0f;
  }
  if (input->move_back.ended_down)
  {
    acceleration.z += 1.0f;
  }
  if (input->move_left.ended_down)
  {
    acceleration.x += -1.0f;
  }
  if (input->move_right.ended_down)
  {
    acceleration.x += 1.0f;
  }
  acceleration = normalize(acceleration);
  if (acceleration != Vec3{})
  {
    auto rot = atan2(-acceleration.x, acceleration.z);
    player->target_rotation = rot;
  }
  f32 direction = wrap_to_neg_pi_to_pi(player->target_rotation - player->rotation);
  player->rotation += direction * PLAYER_ROTATE_SPEED * dt;
  player->rotation = wrap_to_neg_pi_to_pi(player->rotation);

  if (main.camera_mode)
  {
    if (input->camera_move_up.ended_down)
    {
      acceleration.y += 1.0f;
    }
    if (input->camera_move_down.ended_down)
    {
      acceleration.y += -1.0f;
    }

    f32 x_offset = input->mouse_relative.x * CAMERA_SENSITIVITY;
    f32 y_offset = input->mouse_relative.y * CAMERA_SENSITIVITY;
    main.camera.yaw += x_offset;
    main.camera.pitch -= y_offset;
    main.camera.pitch = clamp(main.camera.pitch, -89.0f, 89.0f);
    camera_update_vectors(main.camera);

    // TODO(szulf): i dont know if there is a better way to get this vector,
    // when i just use main.camera.front the movement gets slower the higher/lower you look
    auto forward = cross(CAMERA_WORLD_UP, main.camera.right);
    main.camera.pos += forward * (-acceleration.z * CAMERA_SPEED * dt);
    main.camera.pos += main.camera.right * (acceleration.x * CAMERA_SPEED * dt);
    main.camera.pos += CAMERA_WORLD_UP * (acceleration.y * CAMERA_SPEED * dt);
  }
  else
  {
    acceleration *= PLAYER_MOVEMENT_SPEED;
    // TODO(szulf): just a hack friction, change to proper sometime
    acceleration += -5.0f * player->velocity;
    auto new_position = 0.5f * acceleration * square(dt) + player->velocity * dt + player->position;
    player->velocity = acceleration * dt + player->velocity;

    Vec3 collision_normal = {};
    bool collided = false;
    for (usize i = 0; i < collidables.size; ++i)
    {
      auto& c = *collidables[i];
      if (c.position.y != 0.0f)
      {
        continue;
      }
      Vec3 rounded_pos = {round(new_position.x), 0.0f, round(new_position.z)};
      if ((c.position.x > rounded_pos.x + 1.0f || c.position.x < rounded_pos.x - 1.0f) ||
          (c.position.z > rounded_pos.z + 1.0f || c.position.z < rounded_pos.z - 1.0f))
      {
        continue;
      }

      Entity p = {};
      p.position = new_position;
      p.bounding_box = player->bounding_box;

      if (!entities_collide(p, c))
      {
        continue;
      }

      auto collidable_front = c.position.z + (0.5f * c.bounding_box.depth);
      auto collidable_back = c.position.z - (0.5f * c.bounding_box.depth);
      auto collidable_left = c.position.x - (0.5f * c.bounding_box.width);
      auto collidable_right = c.position.x + (0.5f * c.bounding_box.width);

      auto player_front = p.position.z + (0.5f * p.bounding_box.depth);
      auto player_back = p.position.z - (0.5f * p.bounding_box.depth);
      auto player_left = p.position.x - (0.5f * p.bounding_box.width);
      auto player_right = p.position.x + (0.5f * p.bounding_box.width);

      auto back_overlap = abs(player_back - collidable_front);
      auto front_overlap = abs(player_front - collidable_back);
      auto left_overlap = abs(player_left - collidable_right);
      auto right_overlap = abs(player_right - collidable_left);

      auto collision_overlap =
        min(min(min(back_overlap, front_overlap), left_overlap), right_overlap);

      if (f32_equal(collision_overlap, back_overlap))
      {
        collision_normal.z = -1.0f;
      }
      else if (f32_equal(collision_overlap, front_overlap))
      {
        collision_normal.z = 1.0f;
      }
      else if (f32_equal(collision_overlap, left_overlap))
      {
        collision_normal.x = 1.0f;
      }
      else if (f32_equal(collision_overlap, right_overlap))
      {
        collision_normal.x = -1.0f;
      }
      collided = true;
    }

    auto abs_collision_normal = abs(collision_normal);
    auto collision_normal_inverted = Vec3{1.0f, 0.0f, 1.0f} - abs_collision_normal;
    player->position =
      (abs_collision_normal * player->position) + (new_position * collision_normal_inverted);
    if (collided)
    {
      player->velocity -= dot(player->velocity, collision_normal) * collision_normal;
    }
  }

  if (input->interact.ended_down && input->interact.transition_count != 0)
  {
    for (usize i = 0; i < interactables.size; ++i)
    {
      auto& interactable = *interactables[i];
      auto vec = interactable.position - player->position;
      f32 dist = length2(vec);
      f32 orientation = atan2(-vec.x, vec.z);
      if (dist < interactable_info[interactable.interactable_type].radius2 &&
          (abs(player->rotation - orientation) < 1.0f))
      {
        if (interactable.interactable_type == INTERACTABLE_TYPE_LIGHT_BULB)
        {
          interactable.light_bulb_emissive = !interactable.light_bulb_emissive;
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
    if (main.display_bounding_boxes)
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
