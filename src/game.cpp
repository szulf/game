void clear(Input& input) {
  auto mouse_pos  = input.mouse_pos;
  input           = {};
  input.mouse_pos = mouse_pos;
}

void clear(FrameData& frame) {
  frame.hovered_slot = {};
  frame.ui_cmds.clear();
}

void init(State& state) {
  InitWindow(WINDOW_DIMS.x, WINDOW_DIMS.y, "test");
  SetTargetFPS(165);
  SetExitKey(KEY_NULL);

  auto player_entity = Entity{
    .pos  = {8, 4},
    .data = Player{},
  };
  auto* player = get_data<Player>(player_entity);
  ASSERT(player);
  player->inventory[0]  = ItemSlot{.type = ItemType::CONVEYOR, .count = 85};
  player->inventory[14] = ItemSlot{.type = ItemType::CONVEYOR, .count = 100};
  player->inventory[13] = ItemSlot{.type = ItemType::CONVEYOR, .count = 100};
  player->inventory[8]  = ItemSlot{.type = ItemType::CONVEYOR, .count = 20};
  player->inventory[9]  = ItemSlot{.type = ItemType::BLOCK, .count = 30};
  player->inventory[3]  = ItemSlot{.type = ItemType::STORAGE, .count = 6};
  player->inventory[11] = ItemSlot{.type = ItemType::STORAGE, .count = 11};
  state.player_id       = add_entity(state.store, player_entity);

  add_entity(state.store, Entity{.pos = {5, 9}, .data = Block{}});

  add_entity(state.store, Entity{.pos = {9, 6}, .data = Storage{}});
  add_entity(state.store, Entity{.pos = {9, 10}, .data = Storage{}});
  add_entity(state.store, Entity{.pos = {6, 10}, .data = Storage{}});

  add_entity(state.store, Entity{.pos = {9, 7}, .data = Conveyor{.rotation = Rotation::Down}});
  add_entity(state.store, Entity{.pos = {9, 8}, .data = Conveyor{.rotation = Rotation::Down}});
  add_entity(state.store, Entity{.pos = {9, 9}, .data = Conveyor{.rotation = Rotation::Down}});
  add_entity(state.store, Entity{.pos = {8, 10}, .data = Conveyor{.rotation = Rotation::Left}});
  add_entity(state.store, Entity{.pos = {7, 10}, .data = Conveyor{.rotation = Rotation::Left}});

  flush(state.store);
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

  system_move_player(state.store, state.player_id, state.input);
  system_player_inventory_interactions(
    state.store,
    state.player_id,
    state.frame.hovered_slot,
    state.input
  );
  system_place_entity(state.store, state.player_id, state.input, state.current_place_rotation);
  system_remove_entity(state.store, state.player_id, state.input);
  system_pickup_item(state.store, state.player_id);
  system_move_items(state.store, dt);

  flush(state.store);
  clear_event_bus(state.store);
  clear(state.input);
}

void update_frame(State& state) {
  clear(state.frame);

  state.frame.hovered_slot = system_generate_inventory_uis(
    state.store,
    state.player_id,
    state.input.mouse_pos,
    state.frame.ui_cmds
  );
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
  system_render(state.store);
  // TODO: move to system_render()?
  // TODO: it should also be related to mouse somehow i think
  // TODO: better arrow drawing code?
  // TODO: should not draw when mouse is hovering over some entity
  {
    auto* player = get_data<Player>(state.store, state.player_id);
    ASSERT(player);
    if (player->hand && rotatable(player->hand.type)) {
      static constexpr Color ARROW_COLOR = {80, 60, 0, 255};

      ivec2 main_start_pos = (grid_pos(state.input.mouse_pos) * GRID_DIMS) + (GRID_DIMS / 2);
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

      DrawLineV(vector2_from_ivec2(main_start_pos), vector2_from_ivec2(main_end_pos), ARROW_COLOR);
      DrawLineV(
        vector2_from_ivec2(hands_start_pos),
        vector2_from_ivec2(right_end_pos),
        ARROW_COLOR
      );
      DrawLineV(vector2_from_ivec2(hands_start_pos), vector2_from_ivec2(left_end_pos), ARROW_COLOR);
    }
  }

  // NOTE: ui
  ui::render(state.frame.ui_cmds);
  DrawFPS(5, 5);

  // NOTE: mouse
  {
    if (!state.frame.hovered_slot.entity) {
      Rectangle rect = {
        .x      = f32(grid_pos(state.input.mouse_pos).x * GRID_DIMS.x),
        .y      = f32(grid_pos(state.input.mouse_pos).y * GRID_DIMS.y),
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
    ASSERT(player_entity);
    auto* player = get_data<Player>(*player_entity);
    ASSERT(player);
    auto render = get_render_rect(*player_entity);
    DrawCircleLines(
      (player_entity->pos.x * GRID_DIMS.x) + (render.dims.x * 0.5f),
      (player_entity->pos.y * GRID_DIMS.y) + (render.dims.y * 0.5f),
      player->interaction_radius * GRID_DIMS.x,
      GREEN
    );
  }

  EndDrawing();
}

void shutdown([[maybe_unused]] State& state) {
  CloseWindow();
}
