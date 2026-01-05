#include "base/base.cpp"
#include "platform/platform.h"

static PlatformAPI platform;
static RenderingAPI rendering;

#include "image.cpp"
#include "assets/assets.cpp"
#include "camera.cpp"
#include "renderer/renderer.cpp"
#include "entity.cpp"
#include "data/data.cpp"

struct Main
{
  Allocator allocator;

  assets::Manager assets_manager;

  bool camera_mode;
  bool display_bounding_boxes;

  Camera* main_camera;
  Camera debug_camera;
  Camera gameplay_camera;

  // TODO(szulf): is a map to an array of entities by their type a good idea?
  // it might really be, turns out jon blow uses that
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

  main.assets_manager = assets::manager_make(main.allocator);
  assets::manager_instance = &main.assets_manager;
  renderer::init(main.allocator, error);
  ASSERT(error == SUCCESS, "couldnt initialize renderer");

  main.entities = data::scene_from_file("data/main.gscn", main.allocator, error);
  ASSERT(error == SUCCESS, "couldnt load scene");

  *input = data::keymap_from_file("data/keymap.gkey", error);
  ASSERT(error == SUCCESS, "couldnt read keymap file");

  main.gameplay_camera = {};
  main.gameplay_camera.type = CAMERA_TYPE_PERSPECTIVE;
  main.gameplay_camera.pos = {0.0f, 12.0f, 8.0f};
  main.gameplay_camera.yaw = -90.0f;
  main.gameplay_camera.pitch = -55.0f;
  main.gameplay_camera.near_plane = 0.1f;
  main.gameplay_camera.far_plane = 1000.0f;
  main.gameplay_camera.viewport_width = platform.get_width();
  main.gameplay_camera.viewport_height = platform.get_height();
  main.gameplay_camera.using_vertical_fov = true;
  main.gameplay_camera.fov = 0.25f * F32_PI;
  camera_update_vectors(main.gameplay_camera);

  main.debug_camera = main.gameplay_camera;

  main.main_camera = &main.gameplay_camera;
}

dll_export POST_RELOAD_FN(post_reload)
{
  auto& main = *(Main*) memory->memory;
  assets::manager_instance = &main.assets_manager;
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

  vec3 acceleration = {};
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

  if (main.camera_mode)
  {
    main.main_camera = &main.debug_camera;
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
    main.debug_camera.yaw += x_offset;
    main.debug_camera.pitch -= y_offset;
    main.debug_camera.pitch = clamp(main.debug_camera.pitch, -89.0f, 89.0f);
    camera_update_vectors(main.debug_camera);

    // TODO(szulf): i dont know if there is a better way to get this vector,
    // when i just use main.camera.front the movement gets slower the higher/lower you look
    auto forward = cross(CAMERA_WORLD_UP, main.debug_camera.right);
    main.debug_camera.pos += forward * (-acceleration.z * CAMERA_SPEED * dt);
    main.debug_camera.pos += main.debug_camera.right * (acceleration.x * CAMERA_SPEED * dt);
    main.debug_camera.pos += CAMERA_WORLD_UP * (acceleration.y * CAMERA_SPEED * dt);
  }
  else
  {
    main.main_camera = &main.gameplay_camera;

    // NOTE(szulf): rotation
    {
      if (acceleration != vec3{0.0f, 0.0f, 0.0f})
      {
        auto rot = atan2(-acceleration.x, acceleration.z);
        player->target_rotation = rot;
      }
      f32 direction = wrap_to_neg_pi_to_pi(player->target_rotation - player->rotation);
      player->rotation += direction * PLAYER_ROTATE_SPEED * dt;
      player->rotation = wrap_to_neg_pi_to_pi(player->rotation);
    }

    // NOTE(szulf): movement and collisions
    {
      acceleration *= PLAYER_MOVEMENT_SPEED;
      // TODO(szulf): just a hack friction, change to proper sometime
      acceleration += -5.0f * player->velocity;
      auto new_position =
        0.5f * acceleration * square(dt) + player->velocity * dt + player->position;
      player->velocity = acceleration * dt + player->velocity;

      vec3 collision_normal = {};
      bool collided = false;
      for (usize i = 0; i < collidables.size; ++i)
      {
        auto& c = *collidables[i];
        if (c.position.y != 0.0f)
        {
          continue;
        }
        vec3 rounded_pos = {round(new_position.x), 0.0f, round(new_position.z)};
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
      auto collision_normal_inverted = vec3{1.0f, 0.0f, 1.0f} - abs_collision_normal;
      player->position =
        (abs_collision_normal * player->position) + (new_position * collision_normal_inverted);
      if (collided)
      {
        player->velocity -= dot(player->velocity, collision_normal) * collision_normal;
      }
    }

    // NOTE(szulf): interactions
    {
      if (input->interact.ended_down && input->interact.transition_count != 0)
      {
        for (usize i = 0; i < interactables.size; ++i)
        {
          auto& interactable = *interactables[i];
          auto vec = interactable.position - player->position;
          f32 dist = length2(vec);
          // TODO(szulf): this orientation is kind of annoying,
          // either increase the accepted rad difference,
          // or just remove the whole thing
          f32 orientation = atan2(-vec.x, vec.z);
          if (dist < LIGHT_BULB_RADIUS2 && abs(player->rotation - orientation) < 1.0f &&
              interactable.interactable_type == INTERACTABLE_TYPE_LIGHT_BULB)
          {
            interactable.light_bulb_on = !interactable.light_bulb_on;
            interactable.tint =
              interactable.light_bulb_on ? LIGHT_BULB_TINT_ON : LIGHT_BULB_TINT_OFF;
          }
        }
      }
    }
  }
}

dll_export RENDER_FN(render)
{
  auto& main = *(Main*) memory->memory;

  // TODO(szulf): switch this to the frame arena
  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));

  // NOTE(szulf): shadow map pass
  f32 shadow_map_camera_far_plane;
  {
    // TODO(szulf): change this after changing the entity system
    vec3 pos = {};
    for (usize i = 0; i < main.entities.size; ++i)
    {
      auto& entity = main.entities[i];
      if (entity.type == ENTITY_TYPE_INTERACTABLE &&
          entity.interactable_type == INTERACTABLE_TYPE_LIGHT_BULB)
      {
        pos = entity.position;
      }
    }

    Camera shadow_map_camera = main.gameplay_camera;
    shadow_map_camera.pos = pos;
    shadow_map_camera.pitch = 0.0f;
    shadow_map_camera.yaw = F32_PI;
    shadow_map_camera.near_plane = 0.1f;
    shadow_map_camera.far_plane = shadow_map_camera_far_plane = 25.0f;
    shadow_map_camera.using_vertical_fov = false;
    shadow_map_camera.fov = F32_PI / 2.0f;
    shadow_map_camera.viewport_width = SHADOW_CUBEMAP_WIDTH;
    shadow_map_camera.viewport_height = SHADOW_CUBEMAP_HEIGHT;
    camera_update_vectors(shadow_map_camera);

    mat4 light_proj_mat = camera_projection(shadow_map_camera);
    // TODO(szulf): there is a weird artifact on the player because of the upside down rendering
    mat4 transforms[6] = {
      light_proj_mat * mat4_look_at(pos, pos + vec3{1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * mat4_look_at(pos, pos + vec3{-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * mat4_look_at(pos, pos + vec3{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}),
      light_proj_mat * mat4_look_at(pos, pos + vec3{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}),
      light_proj_mat * mat4_look_at(pos, pos + vec3{0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * mat4_look_at(pos, pos + vec3{0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}),
    };

    auto pass = renderer::pass_make(scratch_arena.allocator);
    pass.camera = shadow_map_camera;
    pass.override_shader = true;
    pass.shader = assets::SHADER_SHADOW_DEPTH;
    pass.framebuffer_id = renderer::shadow_framebuffer_id;
    pass.width = SHADOW_CUBEMAP_WIDTH;
    pass.height = SHADOW_CUBEMAP_HEIGHT;
    pass.transforms_count = 6;
    pass.transforms = transforms;

    for (usize i = 0; i < main.entities.size; ++i)
    {
      auto& entity = main.entities[i];
      if (entity.has_model && entity.type == ENTITY_TYPE_PLAYER)
      {
        auto items = renderer_item_entity(entity, scratch_arena.allocator);
        renderer::queue_items(pass, items);
      }
    }

    renderer::sort_items(pass);
    renderer::draw(pass);
  }

  // NOTE(szulf): main draw pass
  {
    auto pass = renderer::pass_make(scratch_arena.allocator);
    pass.camera = *main.main_camera;
    pass.shadow_map = &renderer::shadow_cubemap;
    pass.shadow_map_camera_far_plane = shadow_map_camera_far_plane;
    pass.width = platform.get_width();
    pass.height = platform.get_height();

    for (usize i = 0; i < main.entities.size; ++i)
    {
      auto& entity = main.entities[i];
      if (entity.has_model)
      {
        auto renderer_items = renderer_item_entity(entity, scratch_arena.allocator);
        renderer::queue_items(pass, renderer_items);
      }
      if (entity.type == ENTITY_TYPE_INTERACTABLE &&
          entity.interactable_type == INTERACTABLE_TYPE_LIGHT_BULB && entity.light_bulb_on)
      {
        renderer::Light light = {};
        light.pos = entity.position;
        light.pos.y += entity.light_bulb_height_offset;
        light.color = entity.light_bulb_color;
        array_push(pass.lights, light);
      }
      if (main.display_bounding_boxes)
      {
        auto bounding_box_items =
          renderer_item_entity_bounding_box(entity, scratch_arena.allocator);
        renderer::queue_items(pass, bounding_box_items);

        if (entity.type == ENTITY_TYPE_INTERACTABLE)
        {
          auto radius_items =
            renderer_item_entity_interactable_radius(entity, scratch_arena.allocator);
          renderer::queue_items(pass, radius_items);
        }
      }
    }

    renderer::sort_items(pass);
    renderer::draw(pass);
  }
}

// TODO(szulf): get rid of this, just set width and height every frame
dll_export EVENT_FN(event)
{
  auto& main = *(Main*) memory->memory;

  switch (event->type)
  {
    case EVENT_TYPE_WINDOW_RESIZE:
    {
      main.gameplay_camera.viewport_width = event->data.window_resize.width;
      main.gameplay_camera.viewport_height = event->data.window_resize.height;
      main.debug_camera.viewport_width = event->data.window_resize.width;
      main.debug_camera.viewport_height = event->data.window_resize.height;
    }
    break;
  }
}
