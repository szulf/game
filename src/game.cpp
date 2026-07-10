#include "core.cpp"

struct Input {
  vec2 mouse_pos{};
  i32 mouse_scroll{};
  bool lmb_pressed{};
  bool rmb_pressed{};

  vec2 move{};
  bool interact{};
  bool close_inv{};
  bool rotate{};
};

void clear(Input& input) {
  auto mouse_pos  = input.mouse_pos;
  input           = {};
  input.mouse_pos = mouse_pos;
}

#include "assets.cpp"
#include "ui.cpp"
#include "items.cpp"
#include "entity.cpp"

struct ItemSlotIdx {
  EntityId entity{};
  u32 slot_idx{};
};

struct FrameData {
  ItemSlotIdx hovered_slot{};
};

void clear(FrameData& frame) {
  frame.hovered_slot = {};
}

struct State {
  Input frame_input{};
  Input tick_input{};

  FrameData frame{};
  AssetManager assets{};
  UI_System ui_system{};

  f32 minutes_accumulator{};
  u64 minutes{};

  ResourceMessageQueue resource_message_queue{};

  // TODO: should reset when changing the player hand item
  Rotation current_place_rotation{};

  // NOTE: there is only one player
  EntityId player_id{};
  EntityStore store{};

  bool debug{};
};

#include "systems.cpp"

void init(State& state) {
  InitWindow(WINDOW_DIMS.x, WINDOW_DIMS.y, "test");
  SetTargetFPS(165);
  SetExitKey(KEY_NULL);

  load_textures(state.assets);

  auto player_entity = Entity{
    .pos  = {8, 4},
    .data = Player{},
  };
  auto* player = get_data<Player>(player_entity);
  ASSERT_NO_MSG(player);
  player->inventory[0]  = ItemSlot{.type = ITEM_CONVEYOR, .count = 85};
  player->inventory[14] = ItemSlot{.type = ITEM_CONVEYOR, .count = 100};
  player->inventory[13] = ItemSlot{.type = ITEM_CONVEYOR, .count = 100};
  player->inventory[8]  = ItemSlot{.type = ITEM_CONVEYOR, .count = 20};
  player->inventory[9]  = ItemSlot{.type = ITEM_BLOCK, .count = 30};
  player->inventory[3]  = ItemSlot{.type = ITEM_STORAGE, .count = 6};
  player->inventory[11] = ItemSlot{.type = ITEM_STORAGE, .count = 11};
  state.player_id       = add_entity(state.store, player_entity);

  add_entity(state.store, Entity{.pos = {5, 9}, .data = Block{}});

  add_entity(state.store, Entity{.pos = {9, 6}, .data = Storage{}});
  add_entity(state.store, Entity{.pos = {9, 10}, .data = Storage{}});
  add_entity(state.store, Entity{.pos = {6, 10}, .data = Storage{}});

  add_entity(state.store, Entity{.pos = {9, 7}, .data = Conveyor{.rotation = Rotation::DOWN}});
  add_entity(state.store, Entity{.pos = {9, 8}, .data = Conveyor{.rotation = Rotation::DOWN}});
  add_entity(state.store, Entity{.pos = {9, 9}, .data = Conveyor{.rotation = Rotation::DOWN}});
  add_entity(state.store, Entity{.pos = {8, 10}, .data = Conveyor{.rotation = Rotation::LEFT}});
  add_entity(state.store, Entity{.pos = {7, 10}, .data = Conveyor{.rotation = Rotation::LEFT}});

  add_entity(state.store, Entity{.pos = {10, 10}, .data = WorldTunnel{.to = World::OTHER}});
  add_entity(
    state.store,
    Entity{.pos = {10, 10}, .world = World::OTHER, .data = WorldTunnel{.to = World::OVERWORLD}}
  );

  add_entity(state.store, Entity{.pos = {12, 10}, .data = ResourceMessageSender{}});
  add_entity(state.store, Entity{.pos = {14, 11}, .data = ResourceMessageReceiver{}});

  flush(state.store);
}

void gather_input(State& state) {
  clear(state.frame_input);

  state.frame_input.mouse_pos = vec2_from_vector2(GetMousePosition());
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    state.frame_input.lmb_pressed = true;
  }
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    state.frame_input.rmb_pressed = true;
  }

  if (IsKeyPressed(KEY_W)) {
    --state.frame_input.move.y;
  }
  if (IsKeyPressed(KEY_S)) {
    ++state.frame_input.move.y;
  }
  if (IsKeyPressed(KEY_A)) {
    --state.frame_input.move.x;
  }
  if (IsKeyPressed(KEY_D)) {
    ++state.frame_input.move.x;
  }
  state.frame_input.move.x = std::clamp(state.frame_input.move.x, -1.0f, 1.0f);
  state.frame_input.move.y = std::clamp(state.frame_input.move.y, -1.0f, 1.0f);

  if (IsKeyPressed(KEY_E)) {
    state.frame_input.interact = true;
  }
  if (IsKeyPressed(KEY_ESCAPE)) {
    state.frame_input.close_inv = true;
  }

  if (IsKeyPressed(KEY_R)) {
    state.frame_input.rotate = true;
  }

  if (IsKeyPressed(KEY_F1)) {
    state.debug = !state.debug;
  }

  state.tick_input.mouse_pos = state.frame_input.mouse_pos;
  state.tick_input.mouse_scroll += state.frame_input.mouse_scroll;
  state.tick_input.lmb_pressed = state.tick_input.lmb_pressed || state.frame_input.lmb_pressed;
  state.tick_input.rmb_pressed = state.tick_input.rmb_pressed || state.frame_input.rmb_pressed;
  state.tick_input.move.x =
    std::clamp(state.tick_input.move.x + state.frame_input.move.x, -1.0f, 1.0f);
  state.tick_input.move.y =
    std::clamp(state.tick_input.move.y + state.frame_input.move.y, -1.0f, 1.0f);
  state.tick_input.interact  = state.tick_input.interact || state.frame_input.interact;
  state.tick_input.close_inv = state.tick_input.close_inv || state.frame_input.close_inv;
  state.tick_input.rotate    = state.tick_input.rotate || state.frame_input.rotate;
}

void update_tick(State& state, f32 dt) {
  // TODO: i dont think this belongs in a system, but maybe?
  if (state.tick_input.rotate) {
    state.current_place_rotation =
      Rotation((i32(state.current_place_rotation) + 1) % i32(Rotation::COUNT));
  }

  system_update_time(state.minutes, state.minutes_accumulator, dt);
  system_move_player(state.store, state.player_id, state.tick_input);
  system_open_gui(state.store, state.player_id, state.tick_input);
  system_close_gui(state.store, state.player_id, state.tick_input);
  system_hand_slot_interactions(
    state.store,
    state.player_id,
    state.frame.hovered_slot,
    state.tick_input
  );
  system_drop_items(state.store, state.player_id, state.tick_input);
  system_place_entity(state.store, state.player_id, state.tick_input, state.current_place_rotation);
  system_remove_entity(state.store, state.player_id, state.tick_input);
  system_pickup_item(state.store, state.player_id);
  system_move_items(state.store, dt);
  system_tunnel_through_worlds(state.store, state.player_id);

  flush(state.store);
  clear_event_bus(state.store);
  clear(state.tick_input);
}

void update_frame(State& state) {
  clear(state.frame);
  ui_system_update(state.ui_system);

  state.frame.hovered_slot = system_inventory_uis(
    state.ui_system,
    state.assets,
    state.frame_input,
    state.store,
    state.player_id
  );

  system_message_sender_ui(
    state.ui_system,
    state.frame_input,
    state.assets,
    state.store,
    state.player_id,
    state.resource_message_queue,
    state.minutes
  );
  auto receiver_hovered_slot = system_message_receiver_ui(
    state.ui_system,
    state.frame_input,
    state.assets,
    state.store,
    state.player_id,
    state.resource_message_queue
  );
  if (receiver_hovered_slot.entity) {
    state.frame.hovered_slot = receiver_hovered_slot;
  }
}

void render(State& state) {
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
  system_render(state.store, state.player_id, state.assets);
  // TODO: move to system_render()?
  // TODO: it should also be related to mouse somehow i think
  // TODO: better arrow drawing code?
  // TODO: should not draw when mouse is hovering over some entity
  {
    auto* player = get_data<Player>(state.store, state.player_id);
    ASSERT_NO_MSG(player);
    if (player->hand && rotatable(player->hand.type)) {
      static constexpr Color ARROW_COLOR = {80, 60, 0, 255};

      vec2 main_start_pos = (grid_pos(state.frame_input.mouse_pos) * GRID_DIMS) + (GRID_DIMS / 2);
      vec2 main_end_pos   = main_start_pos;

      switch (state.current_place_rotation) {
        case Rotation::UP:
          main_start_pos.y -= GRID_DIMS.y / 3.0f;
          main_end_pos.y += GRID_DIMS.y / 3.0f;
          break;
        case Rotation::DOWN:
          main_start_pos.y += GRID_DIMS.y / 3.0f;
          main_end_pos.y -= GRID_DIMS.y / 3.0f;
          break;
        case Rotation::RIGHT:
          main_start_pos.x += GRID_DIMS.x / 3.0f;
          main_end_pos.x -= GRID_DIMS.x / 3.0f;
          break;
        case Rotation::LEFT:
          main_start_pos.x -= GRID_DIMS.x / 3.0f;
          main_end_pos.x += GRID_DIMS.x / 3.0f;
          break;
        case Rotation::COUNT:
          ASSERT(false, "invalid rotation: Rotation::COUNT");
          break;
      }

      auto& hands_start_pos = main_start_pos;
      vec2 left_end_pos     = hands_start_pos;
      vec2 right_end_pos    = hands_start_pos;

      switch (state.current_place_rotation) {
        case Rotation::UP:
          right_end_pos += vec2{-(GRID_DIMS.x / 4.0f), GRID_DIMS.y / 4.0f};
          left_end_pos += vec2{GRID_DIMS.x / 4.0f, GRID_DIMS.y / 4.0f};
          break;
        case Rotation::DOWN:
          right_end_pos += vec2{GRID_DIMS.x / 4.0f, -(GRID_DIMS.y / 4.0f)};
          left_end_pos += vec2{-(GRID_DIMS.x / 4.0f), -(GRID_DIMS.y / 4.0f)};
          break;
        case Rotation::RIGHT:
          right_end_pos += vec2{-(GRID_DIMS.y / 4.0f), GRID_DIMS.x / 4.0f};
          left_end_pos += vec2{-(GRID_DIMS.y / 4.0f), -(GRID_DIMS.x / 4.0f)};
          break;
        case Rotation::LEFT:
          right_end_pos += vec2{GRID_DIMS.y / 4.0f, -(GRID_DIMS.x / 4.0f)};
          left_end_pos += vec2{GRID_DIMS.y / 4.0f, GRID_DIMS.x / 4.0f};
          break;
        case Rotation::COUNT:
          ASSERT(false, "invalid rotation: Rotation::COUNT");
          break;
      }

      DrawLineV(vector2_from_vec2(main_start_pos), vector2_from_vec2(main_end_pos), ARROW_COLOR);
      DrawLineV(vector2_from_vec2(hands_start_pos), vector2_from_vec2(right_end_pos), ARROW_COLOR);
      DrawLineV(vector2_from_vec2(hands_start_pos), vector2_from_vec2(left_end_pos), ARROW_COLOR);
    }
  }

  // NOTE: ui
  ui_render(state.ui_system);
  auto time_str = std::format(
    "{:02}:{:02} DAY: {}",
    (state.minutes / 60) % 24,
    state.minutes % 60,
    (state.minutes / 60) / 24
  );
  DrawText(time_str.c_str(), 5, 25, 20, DARKGREEN);
  DrawFPS(5, 5);

  // NOTE: mouse
  {
    if (!state.frame.hovered_slot.entity) {
      Rectangle rect = {
        .x      = f32(grid_pos(state.frame_input.mouse_pos).x * GRID_DIMS.x),
        .y      = f32(grid_pos(state.frame_input.mouse_pos).y * GRID_DIMS.y),
        .width  = GRID_DIMS.x,
        .height = GRID_DIMS.y,
      };
      DrawRectangleLinesEx(rect, 2, {80, 60, 0, 255});
    }
  }

  // NOTE: debug
  if (state.debug) {
    // NOTE: this should be a system, but because it is a debug thing i will let it slide
    auto* player_entity = get_entity(state.store, state.player_id);
    ASSERT_NO_MSG(player_entity);
    auto* player = get_data<Player>(*player_entity);
    ASSERT_NO_MSG(player);
    auto& texture = state.assets.textures[get_texture_type(*player_entity)];
    DrawCircleLines(
      (player_entity->pos.x * GRID_DIMS.x) + (texture.width * 0.5f),
      (player_entity->pos.y * GRID_DIMS.y) + (texture.height * 0.5f),
      player->interaction_radius * GRID_DIMS.x,
      GREEN
    );
  }

  EndDrawing();
}

void shutdown(State&) {
  CloseWindow();
}
