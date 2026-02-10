#include "game.h"
#include "assets.h"
#include "camera.h"
#include "renderer.h"
#include "entity.h"

enum class Action
{
  MOVE_FRONT,
  MOVE_BACK,
  MOVE_LEFT,
  MOVE_RIGHT,
  INTERACT,

  CAMERA_MOVE_UP,
  CAMERA_MOVE_DOWN,
  TOGGLE_CAMERA_MODE,
  TOGGLE_DISPLAY_BOUNDING_BOXES,

  COUNT,
};

typedef enum_array<Action, os::Key> Keymap;

Keymap load_gkey(const char* path, Error& out_error)
{
  Keymap keymap = {};
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  auto source = os::read_to_string(path, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, keymap);
  auto lines = source.split('\n', scratch_arena.allocator);

  for (usize line_idx = 0; line_idx < lines.size; ++line_idx)
  {
    auto& line = lines[line_idx];
    auto parts = line.split(':', scratch_arena.allocator);
    ERROR_ASSERT(parts.size == 2, out_error, "gkey decoding error. Invalid line.", keymap);

    auto action = parts[0].trim_whitespace();
    auto key_str = parts[1].trim_whitespace();
    auto key = os::string_to_key(key_str, error);
    ERROR_ASSERT(error == SUCCESS, out_error, error, keymap);

    if (action == "move_front")
    {
      keymap[Action::MOVE_FRONT] = key;
    }
    else if (action == "move_back")
    {
      keymap[Action::MOVE_BACK] = key;
    }
    else if (action == "move_left")
    {
      keymap[Action::MOVE_LEFT] = key;
    }
    else if (action == "move_right")
    {
      keymap[Action::MOVE_RIGHT] = key;
    }
    else if (action == "interact")
    {
      keymap[Action::INTERACT] = key;
    }
    else if (action == "toggle_camera_mode")
    {
      keymap[Action::TOGGLE_CAMERA_MODE] = key;
    }
    else if (action == "toggle_display_bounding_boxes")
    {
      keymap[Action::TOGGLE_DISPLAY_BOUNDING_BOXES] = key;
    }
    else if (action == "camera_move_up")
    {
      keymap[Action::CAMERA_MOVE_UP] = key;
    }
    else if (action == "camera_move_down")
    {
      keymap[Action::CAMERA_MOVE_DOWN] = key;
    }
    else
    {
      out_error = "gkey decoding error. Invalid key.";
      return keymap;
    }
  }

  return keymap;
}

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

  Keymap keymap;

  Error errors[512];
  usize error_count;
};

namespace game
{

void init(Memory& memory, os::Window& window)
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

  main.keymap = load_gkey("data/keymap.gkey", error);
  if (error != SUCCESS)
  {
    main.errors[main.error_count++] = error;
  }

  main.gameplay_camera = {};
  main.gameplay_camera.type = CameraType::PERSPECTIVE;
  main.gameplay_camera.rendered_pos = main.gameplay_camera.prev_pos =
    main.gameplay_camera.pos = {0.0f, 12.0f, 8.0f};
  main.gameplay_camera.yaw = -90.0f;
  main.gameplay_camera.pitch = -55.0f;
  main.gameplay_camera.near_plane = 0.1f;
  main.gameplay_camera.far_plane = 1000.0f;
  main.gameplay_camera.viewport_width = window.dimensions.width;
  main.gameplay_camera.viewport_height = window.dimensions.height;
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

void update_tick(Memory& memory, os::Window& window, f32 dt)
{
  auto& main = *(Main*) memory.memory;

  main.gameplay_camera.viewport_width = main.debug_camera.viewport_width = window.dimensions.width;
  main.gameplay_camera.viewport_height = main.debug_camera.viewport_height =
    window.dimensions.height;

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  if (window.input[main.keymap[Action::TOGGLE_CAMERA_MODE]].ended_down &&
      window.input[main.keymap[Action::TOGGLE_CAMERA_MODE]].transition_count != 0)
  {
    main.camera_mode = !main.camera_mode;
  }
  if (window.input[main.keymap[Action::TOGGLE_DISPLAY_BOUNDING_BOXES]].ended_down &&
      window.input[main.keymap[Action::TOGGLE_DISPLAY_BOUNDING_BOXES]].transition_count != 0)
  {
    main.display_bounding_boxes = !main.display_bounding_boxes;
  }

  vec3 acceleration = {};
  if (window.input[main.keymap[Action::MOVE_FRONT]].ended_down)
  {
    acceleration.z += -1.0f;
  }
  if (window.input[main.keymap[Action::MOVE_BACK]].ended_down)
  {
    acceleration.z += 1.0f;
  }
  if (window.input[main.keymap[Action::MOVE_LEFT]].ended_down)
  {
    acceleration.x += -1.0f;
  }
  if (window.input[main.keymap[Action::MOVE_RIGHT]].ended_down)
  {
    acceleration.x += 1.0f;
  }
  acceleration = normalize(acceleration);

  if (main.camera_mode)
  {
    main.main_camera = &main.debug_camera;

    window.consume_mouse_pointer();

    main.gameplay_camera.prev_pos = main.gameplay_camera.pos;
    main.debug_camera.prev_pos = main.debug_camera.pos;

    if (window.input[main.keymap[Action::CAMERA_MOVE_UP]].ended_down)
    {
      acceleration.y += 1.0f;
    }
    if (window.input[main.keymap[Action::CAMERA_MOVE_DOWN]].ended_down)
    {
      acceleration.y += -1.0f;
    }

    auto forward = cross(CAMERA_WORLD_UP, main.debug_camera.right);
    main.debug_camera.pos += forward * (-acceleration.z * CAMERA_SPEED * dt);
    main.debug_camera.pos += main.debug_camera.right * (acceleration.x * CAMERA_SPEED * dt);
    main.debug_camera.pos += CAMERA_WORLD_UP * (acceleration.y * CAMERA_SPEED * dt);
  }
  else
  {
    main.main_camera = &main.gameplay_camera;

    window.release_mouse_pointer();

    for (usize i = 0; i < main.scene.entities_count; ++i)
    {
      auto& entity = main.scene.entities[i];
      entity.prev_pos = entity.pos;
      entity.prev_rotation = entity.rotation;

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
        // TODO: this is still a little wrong in some cases,
        // maybe use a better algorithm overall to improve this
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
        if (window.input[main.keymap[Action::INTERACT]].ended_down &&
            window.input[main.keymap[Action::INTERACT]].transition_count != 0)
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

void update_frame(Memory& memory, os::Window& window, f32 alpha)
{
  auto& main = *(Main*) memory.memory;
  if (main.camera_mode)
  {
    f32 x_offset = window.input.mouse_delta.x * CAMERA_SENSITIVITY;
    f32 y_offset = window.input.mouse_delta.y * CAMERA_SENSITIVITY;
    main.debug_camera.yaw += x_offset;
    main.debug_camera.pitch -= y_offset;
    main.debug_camera.pitch = clamp(main.debug_camera.pitch, -89.0f, 89.0f);
    main.debug_camera.update_vectors();

    main.debug_camera.rendered_pos =
      main.debug_camera.pos * alpha + main.debug_camera.prev_pos * (1.0f - alpha);
  }
  else
  {
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
    auto pass = main.renderer.begin_pass(
      RenderPassType::FORWARD,
      *main.main_camera,
      scratch_arena.allocator,
      main.scene.ambient_color
    );
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
