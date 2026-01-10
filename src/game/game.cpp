#include "base/base.cpp"
#include "platform/platform.h"

static RenderingAPI rendering;

#include "image.cpp"
#include "assets/assets.cpp"
#include "camera.cpp"
#include "renderer/renderer.cpp"
#include "entity.cpp"
#include "data/data.cpp"

// NOTE(szulf): entities[EntityType::PLAYER].size should always be 1
// NOTE(szulf): entities[EntityType::LIGHT_BULB].size should always be 1 (or 0)
struct Entities
{
  Array<Entity> data[(usize) EntityType::COUNT];

  force_inline Array<Entity>& operator[](EntityType idx)
  {
    return data[(usize) idx];
  }

  const force_inline Array<Entity>& operator[](EntityType idx) const
  {
    return data[(usize) idx];
  }
};

struct Main
{
  Allocator allocator;

  assets::Manager assets_manager;

  bool camera_mode;
  bool display_bounding_boxes;

  Camera* main_camera;
  Camera debug_camera;
  Camera gameplay_camera;

  Entities entities;
};

namespace game
{

void spec(Spec& spec)
{
  spec.window_name = "game";
  spec.width = 1280;
  spec.height = 720;
  spec.memory_size = GB(2);
}

void apis(RenderingAPI& rendering_api)
{
  rendering = rendering_api;
}

void init(Memory& memory, Input& input)
{
  Error error = SUCCESS;
  auto& main = *(Main*) memory.memory;

  main.allocator.size = GB(1);
  main.allocator.buffer = (u8*) memory.memory + sizeof(Main);
  main.allocator.type = AllocatorType::ARENA;

  main.assets_manager = assets::manager_make(main.allocator);
  assets::manager_instance = &main.assets_manager;
  renderer::init(main.allocator, error);
  ASSERT(error == SUCCESS, "couldnt initialize renderer");

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  // TODO(szulf): load this from a file directly, after switching to toml
  main.entities[EntityType::PLAYER] = Array<Entity>::make(ArrayType::STATIC, 1, main.allocator);
  main.entities[EntityType::INTERACTABLE] =
    Array<Entity>::make(ArrayType::STATIC, 1, main.allocator);
  main.entities[EntityType::STATIC_COLLISION] =
    Array<Entity>::make(ArrayType::STATIC, 150, main.allocator);

  auto entities = data::scene_from_file("data/main.gscn", main.allocator, error);
  ASSERT(error == SUCCESS, "couldnt load scene");
  for (usize i = 0; i < entities.size; ++i)
  {
    auto& entity = entities[i];
    main.entities[entity.type].push(entity);
  }

  input = data::keymap_from_file("data/keymap.gkey", error);
  ASSERT(error == SUCCESS, "couldnt read keymap file");

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
}

void update(Memory& memory, Input& input, float dt)
{
  auto& main = *(Main*) memory.memory;

  main.gameplay_camera.viewport_width = main.debug_camera.viewport_width = platform::get_width();
  main.gameplay_camera.viewport_height = main.debug_camera.viewport_height = platform::get_height();

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  Entity& player = main.entities[EntityType::PLAYER][0];
  auto& interactables = main.entities[EntityType::INTERACTABLE];

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

    // NOTE(szulf): rotation
    {
      if (acceleration != vec3{0.0f, 0.0f, 0.0f})
      {
        auto rot = atan2(-acceleration.x, acceleration.z);
        player.target_rotation = rot;
      }
      f32 direction = wrap_to_neg_pi_to_pi(player.target_rotation - player.rotation);
      player.rotation += direction * PLAYER_ROTATE_SPEED * dt;
      player.rotation = wrap_to_neg_pi_to_pi(player.rotation);
    }

    // NOTE(szulf): movement and collisions
    {
      acceleration *= PLAYER_MOVEMENT_SPEED;

      static const f32 friction_coefficient = 0.35f;
      static const f32 normal_force = PLAYER_MASS * F32_G;
      static const f32 friction_magnitude = friction_coefficient * normal_force;

      vec3 friction_dir = -normalize(player.velocity);
      vec3 friction_force = friction_dir * friction_magnitude;

      vec3 drag = -3.0f * player.velocity;
      vec3 friction = (friction_force / PLAYER_MASS) + drag;

      acceleration += friction;
      auto new_pos = 0.5f * acceleration * square(dt) + player.velocity * dt + player.pos;
      player.velocity = acceleration * dt + player.velocity;

      vec3 collision_normal = {};
      bool collided = false;
      for (ENUM_CLASS_ENTRIES(EntityType, entity_type_idx))
      {
        if (entity_type_idx == EntityType::PLAYER)
        {
          continue;
        }

        for (usize i = 0; i < main.entities[entity_type_idx].size; ++i)
        {
          auto& c = main.entities[entity_type_idx][i];
          if (c.pos.y != 0.0f)
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
          p.bounding_box = player.bounding_box;

          if (!entities_collide(p, c))
          {
            continue;
          }

          auto collidable_front = c.pos.z + (0.5f * c.bounding_box.depth);
          auto collidable_back = c.pos.z - (0.5f * c.bounding_box.depth);
          auto collidable_left = c.pos.x - (0.5f * c.bounding_box.width);
          auto collidable_right = c.pos.x + (0.5f * c.bounding_box.width);

          auto player_front = p.pos.z + (0.5f * p.bounding_box.depth);
          auto player_back = p.pos.z - (0.5f * p.bounding_box.depth);
          auto player_left = p.pos.x - (0.5f * p.bounding_box.width);
          auto player_right = p.pos.x + (0.5f * p.bounding_box.width);

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
      }

      auto abs_collision_normal = abs(collision_normal);
      auto collision_normal_inverted = vec3{1.0f, 0.0f, 1.0f} - abs_collision_normal;
      player.pos = (abs_collision_normal * player.pos) + (new_pos * collision_normal_inverted);
      if (collided)
      {
        player.velocity -= dot(player.velocity, collision_normal) * collision_normal;
      }
    }

    // NOTE(szulf): interactions
    {
      if (input.interact.ended_down && input.interact.transition_count != 0)
      {
        for (usize i = 0; i < interactables.size; ++i)
        {
          auto& interactable = interactables[i];
          auto vec = interactable.pos - player.pos;
          f32 dist = length2(vec);
          f32 orientation = atan2(-vec.x, vec.z);
          orientation = wrap_to_neg_pi_to_pi(orientation);
          if (dist < square(interactable.interactable_radius) &&
              abs(player.rotation - orientation) < 1.0f &&
              interactable.interactable_type == InteractableType::LIGHT_BULB)
          {
            interactable.light_bulb_on = !interactable.light_bulb_on;
            interactable.tint =
              interactable.light_bulb_on ? LIGHT_BULB_ON_TINT : LIGHT_BULB_OFF_TINT;
          }
        }
      }
    }
  }
}

void render(Memory& memory)
{
  auto& main = *(Main*) memory.memory;

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  // NOTE(szulf): shadow map pass
  f32 shadow_map_camera_far_plane;
  {
    vec3 pos = {};
    for (usize i = 0; i < main.entities[EntityType::INTERACTABLE].size; ++i)
    {
      auto& entity = main.entities[EntityType::INTERACTABLE][i];
      if (entity.interactable_type == InteractableType::LIGHT_BULB)
      {
        pos = entity.pos;
        break;
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
    shadow_map_camera.update_vectors();

    mat4 light_proj_mat = shadow_map_camera.projection();
    // TODO(szulf): there is a weird artifact on the player because of the upside down rendering
    mat4 transforms[6] = {
      light_proj_mat * mat4::look_at(pos, pos + vec3{1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * mat4::look_at(pos, pos + vec3{-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * mat4::look_at(pos, pos + vec3{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}),
      light_proj_mat * mat4::look_at(pos, pos + vec3{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}),
      light_proj_mat * mat4::look_at(pos, pos + vec3{0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}),
      light_proj_mat * mat4::look_at(pos, pos + vec3{0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}),
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

    {
      auto& entity = main.entities[EntityType::PLAYER][0];
      auto items = renderer_item_entity(entity, scratch_arena.allocator);
      renderer::queue_items(pass, items);
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
    pass.width = platform::get_width();
    pass.height = platform::get_height();

    for (ENUM_CLASS_ENTRIES(EntityType, entity_type_idx))
    {
      for (usize i = 0; i < main.entities[entity_type_idx].size; ++i)
      {
        auto& entity = main.entities[entity_type_idx][i];
        if (entity.has_model)
        {
          auto renderer_items = renderer_item_entity(entity, scratch_arena.allocator);
          renderer::queue_items(pass, renderer_items);
        }
        if (entity.type == EntityType::INTERACTABLE &&
            entity.interactable_type == InteractableType::LIGHT_BULB && entity.light_bulb_on)
        {
          renderer::Light light = {};
          light.pos = entity.pos;
          light.pos.y += entity.light_height_offset;
          light.color = entity.light_bulb_color;
          pass.lights.push(light);
        }
        if (main.display_bounding_boxes)
        {
          auto bounding_box_items = renderer_item_entity_bounding_box(entity);
          renderer::queue_items(pass, bounding_box_items);

          switch (entity.type)
          {
            case EntityType::INTERACTABLE:
            {
              auto radius_items = renderer_item_entity_interactable_radius(entity);
              renderer::queue_items(pass, radius_items);
            }
            break;

            case EntityType::PLAYER:
            {
              auto rotation_items = renderer_item_player_rotation(entity);
              renderer::queue_items(pass, rotation_items);
            }
            break;

            case EntityType::STATIC_COLLISION:
            case EntityType::COUNT:
            default:
            {
            }
            break;
          }
        }
      }
    }

    renderer::sort_items(pass);
    renderer::draw(pass);
  }
}

}
