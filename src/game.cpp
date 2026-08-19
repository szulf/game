#include "game.h"

#include "raylib.h"

#include "core.h"
#include "math.h"
#include "utils.h"
#include "input.h"
#include "assets.h"
#include "ui.h"
#include "items.h"
#include "entity.h"
#include "systems.h"
#include "gui.h"
#include "editor.h"
#include "serialization.h"

void init(State& state) {
  state.frame.window_dims = {1280, 720};
  InitWindow(state.frame.window_dims.x, state.frame.window_dims.y, "test");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(165);
  SetExitKey(KEY_NULL);

  load_textures(state.assets);

  state.maintenance_minigame_texture =
    LoadRenderTexture(MAINTENANCE_MINIGAME_DIMS.x, MAINTENANCE_MINIGAME_DIMS.y);

  load_state_from_file(state, DEFAULT_MAP_FILEPATH);
  std::println("loaded world file from '{}'", DEFAULT_MAP_FILEPATH);

  flush(state.store);

  state.camera.zoom = 1.0f;
  // TODO: dont like calling a system in init, but should be fine,
  // and will probably change if i ever introduce a start menu or something
  system_update_camera(
    state.camera,
    state.tick_input,
    state.store,
    state.player_id,
    state.frame.window_dims
  );
}

void update_tick(State& state, f32 dt) {
  if (action_state(state.tick_input, ACTION_TOGGLE_EDITOR_MODE).pressed()) {
    if (state.mode == MODE_EDITOR) {
      state.mode = MODE_GAME;
    } else {
      state.mode = MODE_EDITOR;
    }
  }

  switch (state.mode) {
    case MODE_GAME: {
      // TODO: i dont think this belongs in a system, but maybe?
      if (action_state(state.tick_input, ACTION_ROTATE).pressed()) {
        state.current_place_rotation = next_direction(state.current_place_rotation);
      }

      system_update_time(state.minutes, state.minutes_accumulator, dt);
      system_move_player(state.store, state.player_id, state.tick_input, dt);
      system_open_gui(state.store, state.player_id, state.tick_input, state.frame.mouse_world_pos);
      system_close_gui(state.store, state.player_id, state.tick_input);
      system_hand_slot_interactions(
        state.store,
        state.player_id,
        state.frame.hovered_slot,
        state.tick_input
      );
      system_drop_items(
        state.store,
        state.player_id,
        state.tick_input,
        state.frame.mouse_world_pos
      );
      system_place_entity(
        state.store,
        state.player_id,
        state.tick_input,
        state.frame.mouse_world_pos,
        state.current_place_rotation
      );
      system_remove_entity(
        state.store,
        state.player_id,
        state.tick_input,
        state.frame.mouse_world_pos
      );
      system_pickup_item(state.store, state.player_id);
      system_output_items(state.store, dt);
      system_move_items(state.store, dt);
      system_tunnel_through_worlds(state.store, state.player_id);
      system_transfer_resource_messages(
        state.store,
        state.resource_message_receiver_id,
        state.resource_message_queue,
        state.minutes
      );
      system_progress_recipes(state.store, dt);
      system_apply_maintenance(state.store);
      system_update_maintenance_minigames(state.store, state.tick_input, dt);
      system_update_camera(
        state.camera,
        state.tick_input,
        state.store,
        state.player_id,
        state.frame.window_dims
      );
    } break;
    case MODE_EDITOR: {
      auto result =
        editor_update(state.editor, state.store, state.tick_input, state.frame.mouse_world_pos);
      if (result.player_id) {
        state.player_id = result.player_id;
      }
      if (result.resource_message_receiver_id) {
        state.resource_message_receiver_id = result.resource_message_receiver_id;
      }
    } break;
  }

  if (action_state(state.tick_input, ACTION_SERIALIZE).pressed()) {
    save_state_to_file(state, SERIALIZATION_MAP_FILEPATH);
  }
  if (action_state(state.tick_input, ACTION_DESERIALIZE).pressed()) {
    load_state_from_file(state, SERIALIZATION_MAP_FILEPATH);
  }

  flush(state.store);
  clear_event_bus(state.store);
  clear(state.tick_input);
}

void update_frame(State& state) {
  state.frame             = {};
  state.frame.window_dims = {f32(GetScreenWidth()), f32(GetScreenHeight())};
  state.frame.mouse_world_pos =
    vec2_from_raylib(GetScreenToWorld2D(vec2_to_raylib(state.frame_input.mouse_pos), state.camera));
  gather_input(state.frame_input);
  accumulate_input(state.tick_input, state.frame_input);
  ui_system_update(state.ui_system);

  auto root_layout =
    ui_layout_begin("root", state.ui_system, state.frame_input, {}, state.frame.window_dims);
  ui_element_begin(root_layout, UI_AUTO_ID);

  switch (state.mode) {
    case MODE_GAME: {
      auto player_inv_hovered_slot =
        gui_player_inventory(root_layout, state.assets, state.store, state.player_id);
      if (player_inv_hovered_slot) {
        state.frame.hovered_slot = player_inv_hovered_slot;
      }

      auto open_inventory_hovered_slot =
        gui_open_inventory(root_layout, state.assets, state.store, state.player_id);
      if (open_inventory_hovered_slot) {
        state.frame.hovered_slot = open_inventory_hovered_slot;
      }

      gui_player_hand(
        state.ui_system,
        state.assets,
        state.frame_input,
        state.store,
        state.player_id
      );

      gui_message_sender(
        root_layout,
        state.maintenance_minigame_texture,
        state.assets,
        state.store,
        state.player_id,
        state.resource_message_queue,
        state.minutes
      );

      auto receiver_hovered_slot = gui_message_receiver(
        root_layout,
        state.maintenance_minigame_texture,
        state.assets,
        state.store,
        state.player_id,
        state.resource_message_queue
      );
      if (receiver_hovered_slot) {
        state.frame.hovered_slot = receiver_hovered_slot;
      }

      auto assembler_hovered_slot = gui_assembler(
        root_layout,
        state.maintenance_minigame_texture,
        state.assets,
        state.store,
        state.player_id
      );
      if (assembler_hovered_slot) {
        state.frame.hovered_slot = assembler_hovered_slot;
      }
    } break;
    case MODE_EDITOR: {
      auto result =
        editor_gui(state.editor, root_layout, state.frame_input, state.store, state.assets);
      if (result.save_requested) {
        save_state_to_file(state, DEFAULT_MAP_FILEPATH);
        std::println("saved state to '{}'", DEFAULT_MAP_FILEPATH);
      }
    } break;
  }

  ui_element_end(
    root_layout,
    {
      .layout_direction = UI_LAYOUT_DIRECTION_STACK,
      .sizing           = {ui_sizing_fill(), ui_sizing_fill()},
    }
  );
  ui_layout_end(root_layout);
}

void render(State& state) {
  BeginDrawing();
  ClearBackground(WHITE);

  BeginMode2D(state.camera);

  // NOTE: grid
  {
    static constexpr Color GRID_COLOR = {180, 180, 180, 255};
    for (i32 x = 0; x < state.frame.window_dims.x; x += GRID_DIMS.x) {
      DrawLine(x, 0, x, state.frame.window_dims.y, GRID_COLOR);
    }
    for (i32 y = 0; y < state.frame.window_dims.y; y += GRID_DIMS.y) {
      DrawLine(0, y, state.frame.window_dims.x, y, GRID_COLOR);
    }
  }

  // NOTE: entities
  switch (state.mode) {
    case MODE_GAME: {
      system_render(state.store, state.player_id, state.assets);
      // TODO: move to system_render()?
      // TODO: it should also be related to mouse somehow i think
      // TODO: better arrow drawing code?
      // TODO: should not draw when mouse is hovering over some entity
      {
        auto* player = get_data<Player>(state.store, state.player_id);
        ASSERT_NO_MSG(player);
        if (player->hand) {
          auto rotates = rotatable(player->hand.type);
          if (rotates && *rotates) {
            static constexpr Color ARROW_COLOR = {80, 60, 0, 255};

            vec2 main_start_pos =
              (grid_pos(state.frame.mouse_world_pos) * GRID_DIMS) + (GRID_DIMS / 2);
            vec2 main_end_pos = main_start_pos;

            switch (state.current_place_rotation) {
              case DIR_UP:
                main_start_pos.y -= GRID_DIMS.y / 3.0f;
                main_end_pos.y += GRID_DIMS.y / 3.0f;
                break;
              case DIR_DOWN:
                main_start_pos.y += GRID_DIMS.y / 3.0f;
                main_end_pos.y -= GRID_DIMS.y / 3.0f;
                break;
              case DIR_RIGHT:
                main_start_pos.x += GRID_DIMS.x / 3.0f;
                main_end_pos.x -= GRID_DIMS.x / 3.0f;
                break;
              case DIR_LEFT:
                main_start_pos.x -= GRID_DIMS.x / 3.0f;
                main_end_pos.x += GRID_DIMS.x / 3.0f;
                break;
            }

            auto& hands_start_pos = main_start_pos;
            vec2 left_end_pos     = hands_start_pos;
            vec2 right_end_pos    = hands_start_pos;

            switch (state.current_place_rotation) {
              case DIR_UP:
                right_end_pos += vec2{-(GRID_DIMS.x / 4.0f), GRID_DIMS.y / 4.0f};
                left_end_pos += vec2{GRID_DIMS.x / 4.0f, GRID_DIMS.y / 4.0f};
                break;
              case DIR_DOWN:
                right_end_pos += vec2{GRID_DIMS.x / 4.0f, -(GRID_DIMS.y / 4.0f)};
                left_end_pos += vec2{-(GRID_DIMS.x / 4.0f), -(GRID_DIMS.y / 4.0f)};
                break;
              case DIR_RIGHT:
                right_end_pos += vec2{-(GRID_DIMS.y / 4.0f), GRID_DIMS.x / 4.0f};
                left_end_pos += vec2{-(GRID_DIMS.y / 4.0f), -(GRID_DIMS.x / 4.0f)};
                break;
              case DIR_LEFT:
                right_end_pos += vec2{GRID_DIMS.y / 4.0f, -(GRID_DIMS.x / 4.0f)};
                left_end_pos += vec2{GRID_DIMS.y / 4.0f, GRID_DIMS.x / 4.0f};
                break;
            }

            DrawLineV(vec2_to_raylib(main_start_pos), vec2_to_raylib(main_end_pos), ARROW_COLOR);
            DrawLineV(vec2_to_raylib(hands_start_pos), vec2_to_raylib(right_end_pos), ARROW_COLOR);
            DrawLineV(vec2_to_raylib(hands_start_pos), vec2_to_raylib(left_end_pos), ARROW_COLOR);
          }
        }
      }
    } break;
    case MODE_EDITOR: {
      editor_render(state.editor, state.store, state.assets);
    } break;
  }

  // NOTE: mouse
  {
    if (!state.frame.hovered_slot) {
      auto mouse_grid_pos = grid_pos(state.frame.mouse_world_pos);
      Rectangle rect      = {
             .x      = f32(mouse_grid_pos.x * GRID_DIMS.x),
             .y      = f32(mouse_grid_pos.y * GRID_DIMS.y),
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

  EndMode2D();

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

  EndDrawing();
}

void shutdown(State&) {
  CloseWindow();
}
