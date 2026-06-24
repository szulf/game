#include "game.h"
#include "ecs.h"

static ivec2 grid_pos(const ivec2& pos) {
  return pos / GRID_DIMS;
}

static bool pos_in_radius(const ivec2& pos, const ivec2& start_pos, i32 radius) {
  auto diff2 = length2(pos - start_pos);
  return diff2 < radius * radius;
}

static void item_slot_icon_ui(
  ecs::World& world,
  ecs::Entity item_entity,
  const ivec2& pos,
  std::vector<ui::Command>& ui_cmds
) {
  // TODO: somehow combine this constant with inventory_ui?
  static constexpr ivec2 CELL_DIMS = GRID_DIMS;
  static constexpr f32 ICON_SCALE  = 0.75f;

  if (item_entity) {
    auto& render_rect = ecs::get<RenderRect>(world, item_entity);
    ivec2 icon_dims   = render_rect.dims * ICON_SCALE;
    ivec2 icon_pos    = pos + ((CELL_DIMS - icon_dims) / 2);

    ui_cmds.push_back(ui::Command{
      .type  = ui::CommandType::Rect,
      .pos   = icon_pos,
      .color = render_rect.color,
      .dims  = icon_dims,
    });

    auto& item = ecs::get<Item>(world, item_entity);
    ui_cmds.push_back(ui::Command{
      .type  = ui::CommandType::Text,
      .pos   = pos,
      .color = BLACK,
      .text  = std::format("{}", item.count),
      .size  = 15,
    });
  }
}

static ItemSlotIdx inventory_ui(
  ecs::World& world,
  ecs::Entity inv_entity,
  const ivec2& pos,
  const ivec2& mouse_pos,
  std::vector<ui::Command>& ui_cmds
) {
  // TODO: somehow combine this constant with item_slot_icon_ui?
  static constexpr ivec2 CELL_DIMS = GRID_DIMS;
  static constexpr i32 ROW_SIZE    = 4;

  ItemSlotIdx hovered = {};
  auto& inv           = ecs::get<Inventory>(world, inv_entity);
  for (u32 i = 0; i < inv.size(); ++i) {
    ivec2 cell     = {i32(i % ROW_SIZE), i32(i / ROW_SIZE)};
    ivec2 cell_pos = pos + (cell * CELL_DIMS) + (cell * 2);

    ui_cmds.push_back(ui::Command{
      .type  = ui::CommandType::Rect,
      .pos   = cell_pos,
      .color = LIGHTGRAY,
      .dims  = CELL_DIMS,
    });

    bool hovered_x = mouse_pos.x > cell_pos.x && mouse_pos.x < (cell_pos.x + CELL_DIMS.x);
    bool hovered_y = mouse_pos.y > cell_pos.y && mouse_pos.y < (cell_pos.y + CELL_DIMS.y);
    if (hovered_x && hovered_y) {
      hovered.entity   = inv_entity;
      hovered.slot_idx = i;
    }

    item_slot_icon_ui(world, inv[i], cell_pos, ui_cmds);
  }
  return hovered;
}

static void
turn_to_item(ecs::World& world, ecs::Entity entity, u32 count, bool remove_position = true) {
  auto component_map = get_component_map(world, entity);
  // TODO: remove take multiple components
  ecs::remove<Rotation, Solid, Breakable>(world, entity);
  if (remove_position) {
    ecs::remove<Position>(world, entity);
  }
  ecs::add(world, entity, Item{.count = count, .component_map = component_map});
}

static void restore_from_item(
  ecs::World& world,
  ecs::Entity entity,
  ecs::ComponentMap& component_map,
  const ivec2& pos,
  Rotation rotation = Rotation::Up
) {
  // TODO: actually get a rotation here
  ecs::apply_from_component_map(
    world,
    entity,
    component_map,
    Position{pos},
    rotation,
    Solid{},
    Breakable{}
  );
  ecs::remove<Item>(world, entity);
}

static bool same_item(ecs::World& world, ecs::Entity a, ecs::Entity b) {
  return (ecs::has<Item>(world, a) && ecs::has<Item>(world, b)) &&
         ecs::get<Item>(world, a).component_map == ecs::get<Item>(world, b).component_map &&
         ecs::components_equal<RenderRect, Inventory>(world, a, b);
}

namespace systems {

void move_player(ecs::World& world, ecs::Entity player_entity, Input& input) {
  if (input.move == ivec2{0, 0}) {
    return;
  }

  auto& player_pos = ecs::get<Position>(world, player_entity);
  auto collided    = ecs::find<Position>(world, [&](ecs::Entity _, Position& pos) {
    return pos == player_pos + input.move;
  });
  bool can_move    = true;

  if (collided) {
    ecs::emit(world, PlayerCollided{.entity = *collided});
    if (ecs::has<Solid>(world, *collided)) {
      can_move = false;
    }
  }
  if (can_move) {
    player_pos += input.move;
  }
}

void player_inventory_interactions(
  ecs::World& world,
  ecs::Entity player_entity,
  ItemSlotIdx& hovered_slot,
  Input& input
) {
  auto& player     = ecs::get<Player>(world, player_entity);
  auto& player_pos = ecs::get<Position>(world, player_entity);

  // NOTE: opening
  if (input.interact &&
      pos_in_radius(grid_pos(input.mouse_pos), player_pos, player.interaction_radius)) {
    auto hovered_entity = ecs::find<Position, Inventory>(
      world,
      [&](ecs::Entity _, Position& pos, [[maybe_unused]] Inventory& inv) {
        return pos == grid_pos(input.mouse_pos);
      }
    );
    if (hovered_entity) {
      player.open_inventory = *hovered_entity;
    }
  }

  // NOTE: closing
  if (player.open_inventory) {
    auto& open_inv_pos = ecs::get<Position>(world, player.open_inventory);
    if (input.close_inv || !pos_in_radius(open_inv_pos, player_pos, player.interaction_radius)) {
      player.open_inventory = ecs::NULL_ENTITY;
    }
  }

  // NOTE: hand slot interactions
  if (hovered_slot.entity) {
    if (input.lmb_pressed) {
      auto& hovered_slot_inv = ecs::get<Inventory>(world, hovered_slot.entity);
      auto& slot             = hovered_slot_inv[hovered_slot.slot_idx];
      if (same_item(world, slot, player.hand)) {
        auto& slot_item = ecs::get<Item>(world, slot);
        auto& hand_item = ecs::get<Item>(world, player.hand);
        if (slot_item.count + hand_item.count > ITEM_MAX_COUNT) {
          hand_item.count = (slot_item.count + hand_item.count) - ITEM_MAX_COUNT;
          slot_item.count = ITEM_MAX_COUNT;
        } else {
          slot_item.count += hand_item.count;
          ecs::destroy(world, player.hand);
        }
      } else {
        std::swap(slot, player.hand);
      }
    }
  }
}

ItemSlotIdx generate_inventory_uis(
  ecs::World& world,
  ecs::Entity player_entity,
  const ivec2& mouse_pos,
  std::vector<ui::Command>& ui_cmds
) {
  auto hovered_slot = inventory_ui(world, player_entity, {10, 580}, mouse_pos, ui_cmds);

  auto& player = ecs::get<Player>(world, player_entity);
  if (player.open_inventory) {
    auto open_inv_hovered_slot =
      inventory_ui(world, player.open_inventory, {10, 300}, mouse_pos, ui_cmds);
    if (open_inv_hovered_slot.entity) {
      hovered_slot = open_inv_hovered_slot;
    }
  }

  // TODO: not sure whether this belongs here
  if (player.hand) {
    item_slot_icon_ui(world, player.hand, mouse_pos, ui_cmds);
  }

  return hovered_slot;
}

void place_entity(
  ecs::World& world,
  ecs::Entity player_entity,
  const Input& input,
  Rotation place_rotation
) {
  auto& player     = ecs::get<Player>(world, player_entity);
  auto& player_pos = ecs::get<Position>(world, player_entity);
  if (input.rmb_pressed && player.hand &&
      pos_in_radius(grid_pos(input.mouse_pos), player_pos, player.interaction_radius) &&
      !ecs::find<Position>(world, [&](ecs::Entity _, Position& pos) {
        return pos == grid_pos(input.mouse_pos);
      })) {
    auto& hand_item = ecs::get<Item>(world, player.hand);
    auto entity     = ecs::duplicate(world, player.hand);

    restore_from_item(
      world,
      entity,
      hand_item.component_map,
      grid_pos(input.mouse_pos),
      place_rotation
    );

    --hand_item.count;
    if (hand_item.count == 0) {
      ecs::destroy(world, player.hand);
    }
  }
}

void remove_entity(ecs::World& world, ecs::Entity player_entity, const Input& input) {
  auto& player     = ecs::get<Player>(world, player_entity);
  auto& player_pos = ecs::get<Position>(world, player_entity);
  if (input.lmb_pressed &&
      pos_in_radius(grid_pos(input.mouse_pos), player_pos, player.interaction_radius)) {
    auto hovered_entity = ecs::find<Position, Breakable>(
      world,
      [&](ecs::Entity _, Position& pos, [[maybe_unused]] Breakable& breakable) {
        return pos == grid_pos(input.mouse_pos);
      }
    );

    if (hovered_entity) {
      turn_to_item(world, *hovered_entity, 1, false);
    }
  }
}

void pickup_item(ecs::World& world, ecs::Entity player_entity) {
  listen<PlayerCollided>(world, [&](PlayerCollided& collided) {
    if (has<Item>(world, collided.entity)) {
      auto& player_inv = ecs::get<Inventory>(world, player_entity);
      // TODO: write a better free slot finding algorithm,
      // it should prefer slots where the same_item already is before empty slots
      for (u32 i = 0; i < player_inv.size(); ++i) {
        if (player_inv[i] == ecs::NULL_ENTITY) {
          ecs::remove<Position>(world, collided.entity);
          player_inv[i] = collided.entity;
          break;
        } else if (same_item(world, collided.entity, player_inv[i])) {
          auto& item_collided  = ecs::get<Item>(world, collided.entity);
          auto& item_inventory = ecs::get<Item>(world, player_inv[i]);
          if (item_inventory.count + item_collided.count > ITEM_MAX_COUNT) {
            item_collided.count -= ITEM_MAX_COUNT - item_inventory.count;
            item_inventory.count = ITEM_MAX_COUNT;
          } else {
            item_inventory.count += item_collided.count;
            ecs::destroy(world, collided.entity);
            break;
          }
        }
      }
    }
  });
}

void render(ecs::World& world) {
  static constexpr f32 ITEM_SCALE = 0.625f;

  ecs::query<Position, RenderRect>(
    world,
    [&world](ecs::Entity entity, Position& pos, RenderRect& render) {
      Rectangle rect = {
        .x = f32((pos.x * GRID_DIMS.x) + ((GRID_DIMS.x - render.dims.x) / 2)) +
             (f32(render.dims.x) * 0.5f),
        .y = f32((pos.y * GRID_DIMS.y) + ((GRID_DIMS.y - render.dims.y) / 2)) +
             (f32(render.dims.y) * 0.5f),
        .width  = f32(render.dims.x),
        .height = f32(render.dims.y),
      };
      Vector2 origin = vector2_from_ivec2(render.dims) * 0.5f;

      f32 rotation = 0;
      if (ecs::has<Rotation>(world, entity)) {
        rotation = rotation_degrees(ecs::get<Rotation>(world, entity));
      }

      if (ecs::has<Item>(world, entity)) {
        rect.width *= ITEM_SCALE;
        rect.height *= ITEM_SCALE;
        origin *= ITEM_SCALE;
      }

      DrawRectanglePro(rect, origin, rotation, render.color);
    }
  );
}

}

void clear(Input& input) {
  auto mouse_pos  = input.mouse_pos;
  input           = {};
  input.mouse_pos = mouse_pos;
}

void clear(FrameData& frame) {
  frame.hovered_slot = {};
  frame.ui_cmds.clear();
}

ecs::Entity spawn_player(ecs::World& world) {
  static constexpr u32 INVENTORY_SIZE = 16;

  auto entity = ecs::create(world);
  ecs::add(world, entity, Player{.interaction_radius = 4});
  ecs::add(world, entity, Position{.x = 8, .y = 4});
  ecs::add(world, entity, RenderRect{.color = MAROON, .dims = GRID_DIMS});
  ecs::add(world, entity, Inventory{std::vector<ecs::Entity>(INVENTORY_SIZE)});

  return entity;
}

ecs::Entity spawn_block(ecs::World& world, const ivec2& pos) {
  auto entity = ecs::create(world);
  ecs::add(world, entity, Position{pos});
  ecs::add(world, entity, RenderRect{.color = GRAY, .dims = GRID_DIMS});
  ecs::add(world, entity, Breakable{});
  ecs::add(world, entity, Solid{});

  return entity;
}

ecs::Entity spawn_storage(ecs::World& world, const ivec2& pos) {
  static constexpr f32 SCALE          = 0.75f;
  static constexpr u32 INVENTORY_SIZE = 32;

  auto entity = ecs::create(world);
  ecs::add(world, entity, Position{pos});
  ecs::add(world, entity, RenderRect{.color = DARKBROWN, .dims = GRID_DIMS * SCALE});
  ecs::add(world, entity, Inventory{std::vector<ecs::Entity>(INVENTORY_SIZE)});
  ecs::add(world, entity, Breakable{});
  ecs::add(world, entity, Solid{});

  return entity;
}

ecs::Entity spawn_conveyor(ecs::World& world, const ivec2& pos, Rotation rotation) {
  auto entity = ecs::create(world);
  ecs::add(world, entity, Position{pos});
  ecs::add(
    world,
    entity,
    RenderRect{.color = YELLOW, .dims = {i32(f32(GRID_DIMS.x) * 0.625f), GRID_DIMS.y}}
  );
  ecs::add(world, entity, rotation);
  ecs::add(world, entity, Breakable{});
  ecs::add(world, entity, Solid{});

  return entity;
}

void init(State& state) {
  InitWindow(WINDOW_DIMS.x, WINDOW_DIMS.y, "test");
  SetTargetFPS(165);
  SetExitKey(KEY_NULL);

  state.player_entity = spawn_player(state.world);

  spawn_block(state.world, {5, 9});

  spawn_storage(state.world, {9, 6});
  spawn_storage(state.world, {9, 10});
  spawn_storage(state.world, {6, 10});

  spawn_conveyor(state.world, {9, 7}, Rotation::Down);
  spawn_conveyor(state.world, {9, 8}, Rotation::Down);
  spawn_conveyor(state.world, {9, 9}, Rotation::Down);
  spawn_conveyor(state.world, {8, 10}, Rotation::Left);
  spawn_conveyor(state.world, {7, 10}, Rotation::Left);

  ecs::flush(state.world);

  auto& player_inv = ecs::get<Inventory>(state.world, state.player_entity);

  auto entity = spawn_conveyor(state.world, {}, {});
  ecs::flush(state.world);
  turn_to_item(state.world, entity, 85);
  player_inv[0] = entity;

  entity = spawn_conveyor(state.world, {}, {});
  ecs::flush(state.world);
  turn_to_item(state.world, entity, 100);
  player_inv[14] = entity;

  entity = spawn_conveyor(state.world, {}, {});
  ecs::flush(state.world);
  turn_to_item(state.world, entity, 100);
  player_inv[13] = entity;

  entity = spawn_conveyor(state.world, {}, {});
  ecs::flush(state.world);
  turn_to_item(state.world, entity, 20);
  player_inv[8] = entity;

  entity = spawn_block(state.world, {});
  ecs::flush(state.world);
  turn_to_item(state.world, entity, 30);
  player_inv[9] = entity;

  entity = spawn_storage(state.world, {});
  ecs::flush(state.world);
  turn_to_item(state.world, entity, 6);
  player_inv[3] = entity;

  entity = spawn_storage(state.world, {});
  ecs::flush(state.world);
  turn_to_item(state.world, entity, 11);
  player_inv[11] = entity;

  ecs::flush(state.world);
}

void gather_input(State& state) {
  state.input.mouse_pos = ivec2_from_vector2(GetMousePosition());
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    state.input.lmb_pressed = true;
  }
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    state.input.rmb_pressed = true;
  }

  if (IsKeyPressed(KEY_W)) {
    --state.input.move.y;
  }
  if (IsKeyPressed(KEY_S)) {
    ++state.input.move.y;
  }
  if (IsKeyPressed(KEY_A)) {
    --state.input.move.x;
  }
  if (IsKeyPressed(KEY_D)) {
    ++state.input.move.x;
  }
  state.input.move.x = std::clamp(state.input.move.x, -1, 1);
  state.input.move.y = std::clamp(state.input.move.y, -1, 1);

  if (IsKeyPressed(KEY_E)) {
    state.input.interact = true;
  }
  if (IsKeyPressed(KEY_ESCAPE)) {
    state.input.close_inv = true;
  }

  if (IsKeyPressed(KEY_R)) {
    state.input.rotate = true;
  }

  if (IsKeyPressed(KEY_F1)) {
    state.debug = !state.debug;
  }
}

void update_tick(State& state, [[maybe_unused]] f32 dt) {
  // TODO: i dont think this belongs in a system, but maybe?
  if (state.input.rotate) {
    state.current_place_rotation =
      Rotation((i32(state.current_place_rotation) + 1) % i32(Rotation::Count));
  }

  systems::move_player(state.world, state.player_entity, state.input);
  systems::player_inventory_interactions(
    state.world,
    state.player_entity,
    state.frame.hovered_slot,
    state.input
  );
  systems::place_entity(
    state.world,
    state.player_entity,
    state.input,
    state.current_place_rotation
  );
  systems::remove_entity(state.world, state.player_entity, state.input);
  systems::pickup_item(state.world, state.player_entity);

  ecs::flush(state.world);
  ecs::clear_event_bus(state.world);
  clear(state.input);
}

void update_frame(State& state) {
  clear(state.frame);

  state.frame.hovered_slot = systems::generate_inventory_uis(
    state.world,
    state.player_entity,
    state.input.mouse_pos,
    state.frame.ui_cmds
  );
}

void render(State& state) {
  auto grid_mouse_pos = state.input.mouse_pos / ivec2{GRID_DIMS.x, GRID_DIMS.y};

  BeginDrawing();
  ClearBackground(WHITE);

  // NOTE: grid
  {
    static constexpr Color GRID_COLOR = {180, 180, 180, 255};
    for (i32 x = 0; x < WINDOW_DIMS.x; x += GRID_DIMS.x) {
      DrawLine(x, 0, x, WINDOW_DIMS.y, GRID_COLOR);
    }
    for (i32 y = 0; y < WINDOW_DIMS.y; y += GRID_DIMS.y) {
      DrawLine(0, y, WINDOW_DIMS.x, y, GRID_COLOR);
    }
  }

  // NOTE: entities
  systems::render(state.world);
  // TODO: move to systems::render()?
  // TODO: it should also be related to mouse somehow i think
  // TODO: better arrow drawing code?
  // TODO: should not draw when mouse is hovering over some entity
  {
    auto& player = ecs::get<Player>(state.world, state.player_entity);
    if (player.hand) {
      auto& hand_item = ecs::get<Item>(state.world, player.hand);
      if (ecs::component_map_has<Rotation>(state.world, hand_item.component_map)) {
        static constexpr Color ARROW_COLOR = {80, 60, 0, 255};

        ivec2 main_start_pos = (grid_mouse_pos * GRID_DIMS) + (GRID_DIMS / 2);
        ivec2 main_end_pos   = main_start_pos;

        switch (state.current_place_rotation) {
          case Rotation::Up:
            main_start_pos.y -= GRID_DIMS.y / 3;
            main_end_pos.y += GRID_DIMS.y / 3;
            break;
          case Rotation::Down:
            main_start_pos.y += GRID_DIMS.y / 3;
            main_end_pos.y -= GRID_DIMS.y / 3;
            break;
          case Rotation::Right:
            main_start_pos.x += GRID_DIMS.x / 3;
            main_end_pos.x -= GRID_DIMS.x / 3;
            break;
          case Rotation::Left:
            main_start_pos.x -= GRID_DIMS.x / 3;
            main_end_pos.x += GRID_DIMS.x / 3;
            break;
          case Rotation::Count:
            ASSERT(false);
            break;
        }

        auto& hands_start_pos = main_start_pos;
        ivec2 left_end_pos    = hands_start_pos;
        ivec2 right_end_pos   = hands_start_pos;

        switch (state.current_place_rotation) {
          case Rotation::Up:
            right_end_pos += ivec2{-(GRID_DIMS.x / 4), GRID_DIMS.y / 4};
            left_end_pos += ivec2{GRID_DIMS.x / 4, GRID_DIMS.y / 4};
            break;
          case Rotation::Down:
            right_end_pos += ivec2{GRID_DIMS.x / 4, -(GRID_DIMS.y / 4)};
            left_end_pos += ivec2{-(GRID_DIMS.x / 4), -(GRID_DIMS.y / 4)};
            break;
          case Rotation::Right:
            right_end_pos += ivec2{-(GRID_DIMS.y / 4), GRID_DIMS.x / 4};
            left_end_pos += ivec2{-(GRID_DIMS.y / 4), -(GRID_DIMS.x / 4)};
            break;
          case Rotation::Left:
            right_end_pos += ivec2{GRID_DIMS.y / 4, -(GRID_DIMS.x / 4)};
            left_end_pos += ivec2{GRID_DIMS.y / 4, GRID_DIMS.x / 4};
            break;
          case Rotation::Count:
            ASSERT(false);
            break;
        }

        DrawLineV(
          vector2_from_ivec2(main_start_pos),
          vector2_from_ivec2(main_end_pos),
          ARROW_COLOR
        );
        DrawLineV(
          vector2_from_ivec2(hands_start_pos),
          vector2_from_ivec2(right_end_pos),
          ARROW_COLOR
        );
        DrawLineV(
          vector2_from_ivec2(hands_start_pos),
          vector2_from_ivec2(left_end_pos),
          ARROW_COLOR
        );
      }
    }
  }

  // NOTE: ui
  ui::render(state.frame.ui_cmds);
  DrawFPS(5, 5);

  // NOTE: mouse
  {
    if (!state.frame.hovered_slot.entity) {
      Rectangle rect = {
        .x      = f32(grid_mouse_pos.x * GRID_DIMS.x),
        .y      = f32(grid_mouse_pos.y * GRID_DIMS.y),
        .width  = GRID_DIMS.x,
        .height = GRID_DIMS.y,
      };
      DrawRectangleLinesEx(rect, 2, {80, 60, 0, 255});
    }
  }

  // NOTE: debug
  if (state.debug) {
    // NOTE: this should be a system, but because it is a debug thing i will let it slide
    auto& player_pos         = ecs::get<Position>(state.world, state.player_entity);
    auto& player             = ecs::get<Player>(state.world, state.player_entity);
    auto& player_render_rect = ecs::get<RenderRect>(state.world, state.player_entity);
    DrawCircleLines(
      (player_pos.x * GRID_DIMS.x) + (player_render_rect.dims.x * 0.5f),
      (player_pos.y * GRID_DIMS.y) + (player_render_rect.dims.y * 0.5f),
      player.interaction_radius * GRID_DIMS.x,
      GREEN
    );
  }

  EndDrawing();
}

void shutdown([[maybe_unused]] State& state) {
  CloseWindow();
}
