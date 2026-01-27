#include "base/base.cpp"
#include "base/math.h"
#include "platform/platform.h"

#include "image.cpp"
#include "assets.cpp"
#include "camera.cpp"
#include "renderer.cpp"
#include "entity.cpp"

struct Main
{
  Allocator allocator;

  Assets assets;
  Renderer renderer;

  bool camera_mode;
  bool display_bounding_boxes;
  bool recording_test;

  Camera* main_camera;
  Camera debug_camera;
  Camera gameplay_camera;

  Scene scene;

  Error errors[512];
  usize error_count;
};

// TODO: i feel like this function should be in a different file
game::Input load_gkey(const char* path, Error& out_error)
{
  game::Input input = {};
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto source = platform::read_file_to_string(path, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, input);
  auto lines = source.split('\n', scratch_arena.allocator);

  for (usize line_idx = 0; line_idx < lines.size; ++line_idx)
  {
    auto& line = lines[line_idx];
    auto parts = line.split(':', scratch_arena.allocator);
    ERROR_ASSERT(parts.size == 2, out_error, "gkey decoding error. Invalid line.", input);

    auto action = parts[0].trim_whitespace();
    auto key_str = parts[1].trim_whitespace();
    auto key = string_to_key(key_str, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, input);

    if (action == "move_front")
    {
      input.move_front.key = key;
    }
    else if (action == "move_back")
    {
      input.move_back.key = key;
    }
    else if (action == "move_left")
    {
      input.move_left.key = key;
    }
    else if (action == "move_right")
    {
      input.move_right.key = key;
    }
    else if (action == "interact")
    {
      input.interact.key = key;
    }
    else if (action == "toggle_camera_mode")
    {
      input.toggle_camera_mode.key = key;
    }
    else if (action == "toggle_display_bounding_boxes")
    {
      input.toggle_display_bounding_boxes.key = key;
    }
    else if (action == "camera_move_up")
    {
      input.camera_move_up.key = key;
    }
    else if (action == "camera_move_down")
    {
      input.camera_move_down.key = key;
    }
    else
    {
      out_error = "gkey decoding error. Invalid key.";
      return input;
    }
  }

  return input;
}

namespace game
{

void spec(Spec& spec)
{
  spec.window_name = "game";
  spec.width = 1280;
  spec.height = 720;
  spec.memory_size = GB(2);
}

void init(Memory& memory, Input& input)
{
  Error error = SUCCESS;
  auto& main = *(Main*) memory.memory;

  main.allocator.size = GB(1);
  main.allocator.buffer = (u8*) memory.memory + sizeof(Main);
  main.allocator.type = AllocatorType::ARENA;

  main.assets = Assets::make(main.allocator);

  main.renderer = Renderer::make(main.assets, main.allocator);

  main.scene = load_gscn("data/main.gscn", main.assets, main.allocator, error);
  if (error != SUCCESS)
  {
    main.errors[main.error_count++] = error;
  }

  input = load_gkey("data/keymap.gkey", error);
  if (error != SUCCESS)
  {
    main.errors[main.error_count++] = error;
  }

  main.gameplay_camera = {};
  main.gameplay_camera.type = CameraType::PERSPECTIVE;
  main.gameplay_camera.pos = {0.0f, 12.0f, 8.0f};
  main.gameplay_camera.yaw = -90.0f;
  main.gameplay_camera.pitch = -55.0f;
  main.gameplay_camera.near_plane = 0.1f;
  main.gameplay_camera.far_plane = 1000.0f;
  main.gameplay_camera.viewport_width = platform::get_width();
  main.gameplay_camera.viewport_height = platform::get_height();
  main.gameplay_camera.using_vertical_fov = true;
  main.gameplay_camera.fov = 0.25f * F32_PI;
  main.gameplay_camera.update_vectors();

  main.debug_camera = main.gameplay_camera;

  main.main_camera = &main.gameplay_camera;

  if (main.error_count != 0)
  {
    print("Encountered %zu errors.\n", main.error_count);
    for (usize i = 0; i < main.error_count; ++i)
    {
      print("%zu: %s\n", i, main.errors[i]);
    }
  }
}

void update_tick(Memory& memory, Input& input, float dt)
{
  auto& main = *(Main*) memory.memory;

  for (usize i = 0; i < main.scene.entities_count; ++i)
  {
    auto& entity = main.scene.entities[i];
    entity.prev_pos = entity.pos;
    entity.prev_rotation = entity.rotation;
  }

  main.gameplay_camera.viewport_width = main.debug_camera.viewport_width = platform::get_width();
  main.gameplay_camera.viewport_height = main.debug_camera.viewport_height = platform::get_height();

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  if (input.toggle_camera_mode.ended_down && input.toggle_camera_mode.transition_count != 0)
  {
    main.camera_mode = !main.camera_mode;
  }
  if (input.toggle_display_bounding_boxes.ended_down &&
      input.toggle_display_bounding_boxes.transition_count != 0)
  {
    main.display_bounding_boxes = !main.display_bounding_boxes;
  }

  vec3 acceleration = {};
  if (input.move_front.ended_down)
  {
    acceleration.z += -1.0f;
  }
  if (input.move_back.ended_down)
  {
    acceleration.z += 1.0f;
  }
  if (input.move_left.ended_down)
  {
    acceleration.x += -1.0f;
  }
  if (input.move_right.ended_down)
  {
    acceleration.x += 1.0f;
  }
  acceleration = normalize(acceleration);

  if (main.camera_mode)
  {
    main.main_camera = &main.debug_camera;
    if (input.camera_move_up.ended_down)
    {
      acceleration.y += 1.0f;
    }
    if (input.camera_move_down.ended_down)
    {
      acceleration.y += -1.0f;
    }

    f32 x_offset = input.mouse_relative.x * CAMERA_SENSITIVITY;
    f32 y_offset = input.mouse_relative.y * CAMERA_SENSITIVITY;
    main.debug_camera.yaw += x_offset;
    main.debug_camera.pitch -= y_offset;
    main.debug_camera.pitch = clamp(main.debug_camera.pitch, -89.0f, 89.0f);
    main.debug_camera.update_vectors();

    auto forward = cross(CAMERA_WORLD_UP, main.debug_camera.right);
    main.debug_camera.pos += forward * (-acceleration.z * CAMERA_SPEED * dt);
    main.debug_camera.pos += main.debug_camera.right * (acceleration.x * CAMERA_SPEED * dt);
    main.debug_camera.pos += CAMERA_WORLD_UP * (acceleration.y * CAMERA_SPEED * dt);
  }
  else
  {
    main.main_camera = &main.gameplay_camera;

    for (usize i = 0; i < main.scene.entities_count; ++i)
    {
      auto& entity = main.scene.entities[i];
      if (entity.controlled_by_player)
      {
        // NOTE: rotation
        {
          if (acceleration != vec3{0.0f, 0.0f, 0.0f})
          {
            auto rot = atan2(-acceleration.x, acceleration.z);
            entity.target_rotation = rot;
          }
          f32 direction = wrap_to_neg_pi_to_pi(entity.target_rotation - entity.rotation);
          entity.rotation += direction * PLAYER_ROTATE_SPEED * dt;
          entity.rotation = wrap_to_neg_pi_to_pi(entity.rotation);
        }

        // NOTE: movement and collisions
        {
          acceleration *= PLAYER_MOVEMENT_SPEED;

          static const f32 friction_coefficient = 0.35f;
          static const f32 normal_force = PLAYER_MASS * F32_G;
          static const f32 friction_magnitude = friction_coefficient * normal_force;

          vec3 friction_dir = -normalize(entity.velocity);
          vec3 friction_force = friction_dir * friction_magnitude;

          vec3 drag = -3.0f * entity.velocity;
          vec3 friction = (friction_force / PLAYER_MASS) + drag;

          acceleration += friction;
          auto new_pos = 0.5f * acceleration * square(dt) + entity.velocity * dt + entity.pos;
          entity.velocity = acceleration * dt + entity.velocity;

          vec3 collision_normal = {};
          bool collided = false;
          // TODO: still wrong? only checking collisions between the player and collidables
          for (usize collidable_idx = 0; collidable_idx < main.scene.entities_count;
               ++collidable_idx)
          {
            auto& c = main.scene.entities[collidable_idx];
            if (&c == &entity || !c.collidable || c.pos.y != 0.0f)
            {
              continue;
            }

            vec3 rounded_pos = {round(new_pos.x), 0.0f, round(new_pos.z)};
            if ((c.pos.x > rounded_pos.x + 1.0f || c.pos.x < rounded_pos.x - 1.0f) ||
                (c.pos.z > rounded_pos.z + 1.0f || c.pos.z < rounded_pos.z - 1.0f))
            {
              continue;
            }

            Entity p = {};
            p.pos = new_pos;
            p.bounding_box = entity.bounding_box;

            if (!entities_collide(p, c))
            {
              continue;
            }

            auto collidable_front = c.pos.z + (0.5f * c.bounding_box.depth);
            auto collidable_back = c.pos.z - (0.5f * c.bounding_box.depth);
            auto collidable_left = c.pos.x - (0.5f * c.bounding_box.width);
            auto collidable_right = c.pos.x + (0.5f * c.bounding_box.width);

            auto entity_front = p.pos.z + (0.5f * p.bounding_box.depth);
            auto entity_back = p.pos.z - (0.5f * p.bounding_box.depth);
            auto entity_left = p.pos.x - (0.5f * p.bounding_box.width);
            auto entity_right = p.pos.x + (0.5f * p.bounding_box.width);

            auto back_overlap = abs(entity_back - collidable_front);
            auto front_overlap = abs(entity_front - collidable_back);
            auto left_overlap = abs(entity_left - collidable_right);
            auto right_overlap = abs(entity_right - collidable_left);

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
          entity.pos = (abs_collision_normal * entity.pos) + (new_pos * collision_normal_inverted);
          if (collided)
          {
            entity.velocity -= dot(entity.velocity, collision_normal) * collision_normal;
          }
        }

        // NOTE: interactions
        if (input.interact.ended_down && input.interact.transition_count != 0)
        {
          for (usize interactable_idx = 0; interactable_idx < main.scene.entities_count;
               ++interactable_idx)
          {
            auto& interactable = main.scene.entities[interactable_idx];
            if (!interactable.interactable)
            {
              continue;
            }
            auto vec = interactable.pos - entity.pos;
            f32 dist = length2(vec);
            f32 orientation = atan2(-vec.x, vec.z);
            orientation = wrap_to_neg_pi_to_pi(orientation);
            if (dist < square(interactable.interactable_radius) &&
                abs(entity.rotation - orientation) < 1.0f)
            {
              // TODO: this is light bulb specific behaviour, how do i work with other
              // interactables?
              interactable.emits_light = !interactable.emits_light;
              interactable.tint =
                interactable.emits_light ? LIGHT_BULB_ON_TINT : LIGHT_BULB_OFF_TINT;
            }
          }
        }
      }
    }
  }
}

void update_frame(Memory& memory, f32 alpha)
{
  auto& main = *(Main*) memory.memory;
  for (usize i = 0; i < main.scene.entities_count; ++i)
  {
    auto& entity = main.scene.entities[i];
    entity.rendered_pos = entity.pos * alpha + entity.prev_pos * (1.0f - alpha);
    vec2 prev_rot_vec = {-sin(entity.prev_rotation), cos(entity.prev_rotation)};
    vec2 rot_vec = {-sin(entity.rotation), cos(entity.rotation)};
    vec2 rendered_rot_vec = rot_vec * alpha + prev_rot_vec * (1.0f - alpha);
    entity.rendered_rotation = atan2(-rendered_rot_vec.x, rendered_rot_vec.y);
  }
}

void render(Memory& memory)
{
  auto& main = *(Main*) memory.memory;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  // NOTE: shadow map pass
  Camera shadow_map_camera = {};
  {
    vec3 pos = {};
    for (usize i = 0; i < main.scene.entities_count; ++i)
    {
      auto& entity = main.scene.entities[i];
      if (entity.emits_light)
      {
        pos = entity.pos;
        pos.y += entity.light_height_offset;
        break;
      }
    }

    shadow_map_camera = main.gameplay_camera;
    shadow_map_camera.pos = pos;
    shadow_map_camera.pitch = 0.0f;
    shadow_map_camera.yaw = F32_PI;
    shadow_map_camera.near_plane = 0.1f;
    shadow_map_camera.far_plane = 25.0f;
    shadow_map_camera.using_vertical_fov = false;
    shadow_map_camera.fov = F32_PI / 2.0f;
    shadow_map_camera.viewport_width = RenderData::SHADOW_CUBEMAP_WIDTH;
    shadow_map_camera.viewport_height = RenderData::SHADOW_CUBEMAP_HEIGHT;
    shadow_map_camera.update_vectors();

    auto pass = main.renderer.begin_pass(
      RenderPassType::POINT_SHADOW_MAP,
      shadow_map_camera,
      scratch_arena.allocator
    );

    for (usize i = 0; i < main.scene.entities_count; ++i)
    {
      auto& entity = main.scene.entities[i];
      if (entity.controlled_by_player && entity.renderable)
      {
        pass.draw_mesh(entity.mesh, entity.rendered_pos, entity.rendered_rotation);
      }
    }

    pass.finish();
  }

  // NOTE: main draw pass
  {
    auto pass =
      main.renderer.begin_pass(RenderPassType::FORWARD, *main.main_camera, scratch_arena.allocator);
    pass.use_shadow_map(shadow_map_camera);

    for (usize i = 0; i < main.scene.entities_count; ++i)
    {
      const auto& entity = main.scene.entities[i];
      if (entity.renderable)
      {
        pass.draw_mesh(entity.mesh, entity.rendered_pos, entity.rendered_rotation, entity.tint);
      }
      if (entity.emits_light)
      {
        vec3 pos = entity.rendered_pos;
        pos.y += entity.light_height_offset;
        pass.set_light(pos, entity.light_color);
      }

      if (main.display_bounding_boxes)
      {
        if (entity.controlled_by_player)
        {
          pass.draw_line(entity.rendered_pos, 0.6f, entity.rendered_rotation, {1.0f, 0.0f, 0.0f});
        }
        if (entity.collidable && f32_equal(entity.pos.y, 0.0f))
        {
          pass.draw_cube_wires(
            entity.rendered_pos,
            {entity.bounding_box.width, 1.0f, entity.bounding_box.depth},
            {0.0f, 1.0f, 0.0f}
          );
        }
        if (entity.interactable)
        {
          pass.draw_ring(entity.rendered_pos, entity.interactable_radius, {1.0f, 1.0f, 0.0f});
        }
      }
    }

    pass.finish();
  }
}

}
