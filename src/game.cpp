#include <fstream>

#include "raylib.h"
#include "json.hpp"
using json = nlohmann::json;

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

// TODO: when deserializing the std::vector's may get a wrong size,
// if i serialized them with one and then i change it to something else,
// the old one will still remain

struct FrameData {
  ItemSlotIdx hovered_slot{};
};

enum Mode {
  MODE_GAME,
  MODE_EDITOR,
};

static constexpr std::string_view DEFAULT_MAP_FILEPATH       = "default_map.json";
static constexpr std::string_view SERIALIZATION_MAP_FILEPATH = "save_file.json";

struct State {
  static constexpr u32 SERIALIZATION_VERSION = 1;
  Mode mode{};

  // TODO: put into FrameData?
  Input frame_input{};
  // TODO: create a struct TickData?
  Input tick_input{};

  FrameData frame{};
  AssetManager assets{};
  UI_System ui_system{};

  f32 minutes_accumulator{};
  u64 minutes{};

  ResourceMessageQueue resource_message_queue{};

  // TODO: should reset when changing the player hand item
  Direction current_place_rotation{};

  // NOTE: there is only one player
  EntityId player_id{};
  // NOTE: there is only one resource message receiver
  EntityId resource_message_receiver_id{};
  EntityStore store{};

  RenderTexture maintenance_minigame_texture{};

  bool debug{};

  editor::Data editor{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(vec2, x, y);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Color, r, g, b, a);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Rectangle, x, y, width, height);

void to_json(json& j, ItemType t) {
  j = get_item_name(t);
}

void from_json(const json& j, ItemType& t) {
  for (u32 i = 0; i < ITEM_COUNT; ++i) {
    if (j == get_item_name(ItemType(i))) {
      t = ItemType(i);
      return;
    }
  }
  ASSERT(false, "invalid json item type");
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ItemSlot, flags, type, count, damage);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EntityId, idx, gen);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConveyorItem, slot, t);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Cogwheel, pos, radius, color);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LubricationPoint, dims, pos, color, progress);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DirtyRect, area, progress);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Component, slot, pos);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentSlot, pos);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ResourceMessage, requested_items, arrival_time, batch_number);

void to_json(json& j, const Maintenance& m) {
  std::visit(
    overloaded{
      [&](const std::monostate&) {
        j = nullptr;
      },
      [&](const MaintenanceLubrication& v) {
        j = json{
          {"type", v.NAME},
          {"open", v.open},
          {"cogwheels", v.cogwheels},
          {"points", v.points},
        };
      },
      [&](const MaintenanceCleaning& v) {
        j = json{
          {"type", v.NAME},
          {"open", v.open},
          {"dirty_rects", v.dirty_rects},
        };
      },
      [&](const MaintenanceComponentReplacement& v) {
        j = json{
          {"type", v.NAME},
          {"open", v.open},
          {"slots", v.slots},
          {"broken", v.broken},
          {"fixed", v.fixed},
        };
      },
      [&](const MaintenanceCalibration& v) {
        j = json{
          {"type", v.NAME},
          {"open", v.open},
          {"range_low", v.range_low},
          {"range_high", v.range_high},
          {"value", v.value},
          {"t", v.t},
        };
      },
      [&](const MaintenanceMessagingSystem& v) {
        j = json{
          {"type", v.NAME},
        };
      },
    },
    m
  );
}

void from_json(const json& j, Maintenance& m) {
  if (j.is_null()) {
    m = std::monostate{};
    return;
  }

  auto type = j.at("type").get<std::string>();
  if (type == MaintenanceLubrication::NAME) {
    MaintenanceLubrication v{};
    j.at("open").get_to(v.open);
    j.at("cogwheels").get_to(v.cogwheels);
    j.at("points").get_to(v.points);
    m = v;
  } else if (type == MaintenanceCleaning::NAME) {
    MaintenanceCleaning v{};
    j.at("open").get_to(v.open);
    j.at("dirty_rects").get_to(v.dirty_rects);
    m = v;
  } else if (type == MaintenanceComponentReplacement::NAME) {
    MaintenanceComponentReplacement v{};
    j.at("open").get_to(v.open);
    j.at("slots").get_to(v.slots);
    j.at("broken").get_to(v.broken);
    j.at("fixed").get_to(v.fixed);
    m = v;
  } else if (type == MaintenanceCalibration::NAME) {
    MaintenanceCalibration v{};
    j.at("open").get_to(v.open);
    j.at("range_low").get_to(v.range_low);
    j.at("range_high").get_to(v.range_high);
    j.at("value").get_to(v.value);
    j.at("t").get_to(v.t);
    m = v;
  } else if (type == MaintenanceMessagingSystem::NAME) {
    MaintenanceMessagingSystem v{};
    m = v;
  }
}

void to_json(json& j, const EntityData& data) {
  std::visit(
    overloaded{
      [&](const Block&) {
        j = json{
          {"type", "block"},
        };
      },
      [&](const Player& v) {
        j = json{
          {"type", "player"},
          {"inventory", v.inventory},
          {"interaction_radius", v.interaction_radius},
          {"open_gui", v.open_gui},
          {"hand", v.hand},
        };
      },
      [&](const Storage& v) {
        j = json{
          {"type", "storage"},
          {"inventory", v.inventory},
        };
      },
      [&](const Conveyor& v) {
        j = json{
          {"type", "conveyor"},
          {"rotation", v.rotation},
          {"items", v.items},
        };
      },
      [&](const Item& v) {
        j = json{
          {"type", "item"},
          {"slot", v.slot},
        };
      },
      [&](const WorldTunnel& v) {
        j = json{
          {"type", "world_tunnel"},
          {"to", v.to},
          {"inventory", v.inventory},
        };
      },
      [&](const ResourceMessageSender& v) {
        j = json{
          {"type", "resource_message_sender"},
          {"maintenance", v.maintenance},
          {"page", v.page},
          {"msg_in_create", v.msg_in_create},
        };
      },
      [&](const ResourceMessageReceiver& v) {
        j = json{
          {"type", "resource_message_receiver"},
          {"maintenance", v.maintenance},
          {"inventory", v.inventory},
        };
      },
      [&](const Assembler& v) {
        j = json{
          {"type", "assembler"},
          {"maintenance", v.maintenance},
          {"selected_recipe_idx", v.selected_recipe_idx},
          {"inventory", v.inventory},
          {"t", v.t},
        };
      },
    },
    data
  );
}

void from_json(const json& j, EntityData& d) {
  auto type = j.at("type").get<std::string>();
  if (type == "block") {
    d = Block{};
  } else if (type == "player") {
    Player v{};
    j.at("inventory").get_to(v.inventory);
    j.at("interaction_radius").get_to(v.interaction_radius);
    j.at("open_gui").get_to(v.open_gui);
    j.at("hand").get_to(v.hand);
    d = v;
  } else if (type == "storage") {
    Storage v{};
    j.at("inventory").get_to(v.inventory);
    d = v;
  } else if (type == "conveyor") {
    Conveyor v{};
    j.at("rotation").get_to(v.rotation);
    j.at("items").get_to(v.items);
    d = v;
  } else if (type == "item") {
    Item v{};
    j.at("slot").get_to(v.slot);
    d = v;
  } else if (type == "world_tunnel") {
    WorldTunnel v{};
    j.at("to").get_to(v.to);
    j.at("inventory").get_to(v.inventory);
    d = v;
  } else if (type == "resource_message_sender") {
    ResourceMessageSender v{};
    j.at("maintenance").get_to(v.maintenance);
    j.at("page").get_to(v.page);
    j.at("msg_in_create").get_to(v.msg_in_create);
    d = v;
  } else if (type == "resource_message_receiver") {
    ResourceMessageReceiver v{};
    j.at("maintenance").get_to(v.maintenance);
    j.at("inventory").get_to(v.inventory);
    d = v;
  } else if (type == "assembler") {
    Assembler v{};
    j.at("maintenance").get_to(v.maintenance);
    j.at("selected_recipe_idx").get_to(v.selected_recipe_idx);
    j.at("inventory").get_to(v.inventory);
    j.at("t").get_to(v.t);
    d = v;
  } else {
    ASSERT(false, "invalid entity data type");
  }
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Entity, id, pos, world, data);
// TODO: could probably only serialize the entities vector
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EntityStore, free_slots, next_entity_idx, entities);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ResourceMessageQueue, msgs);

void to_json(json& j, const State& s) {
  j = json{
    {"version", s.SERIALIZATION_VERSION},
    {"minutes", s.minutes},
    {"resource_message_queue", s.resource_message_queue},
    {"player_id", s.player_id},
    {"resource_message_receiver_id", s.resource_message_receiver_id},
    {"store", s.store},
  };
}

void from_json(const json& j, State& s) {
  ASSERT(j.at("version") == s.SERIALIZATION_VERSION, "invalid serialization version");
  j.at("minutes").get_to(s.minutes);
  j.at("resource_message_queue").get_to(s.resource_message_queue);
  j.at("player_id").get_to(s.player_id);
  j.at("resource_message_receiver_id").get_to(s.resource_message_receiver_id);
  j.at("store").get_to(s.store);
}

void save_state_to_file(const State& state, const std::filesystem::path& filepath) {
  json j(state);
  std::ofstream file{filepath};
  file << std::setw(4) << j << '\n';
}

void load_state_from_file(State& state, const std::filesystem::path& filepath) {
  std::ifstream file{filepath};
  json j         = json::parse(file);
  auto new_state = j.get<State>();

  // TODO: pull this state assigning out to a separate function?
  state.frame_input            = {};
  state.tick_input             = {};
  state.frame                  = {};
  state.ui_system              = {};
  state.minutes_accumulator    = {};
  state.current_place_rotation = {};
  state.debug                  = {};

  state.minutes                      = new_state.minutes;
  state.resource_message_queue       = new_state.resource_message_queue;
  state.player_id                    = new_state.player_id;
  state.resource_message_receiver_id = new_state.resource_message_receiver_id;
  state.store                        = new_state.store;
}

void init(State& state) {
  InitWindow(WINDOW_DIMS.x, WINDOW_DIMS.y, "test");
  SetTargetFPS(165);
  SetExitKey(KEY_NULL);

  load_textures(state.assets);

  state.maintenance_minigame_texture =
    LoadRenderTexture(MAINTENANCE_MINIGAME_DIMS.x, MAINTENANCE_MINIGAME_DIMS.y);

  load_state_from_file(state, DEFAULT_MAP_FILEPATH);
  std::println("loaded world file from '{}'", DEFAULT_MAP_FILEPATH);

  flush(state.store);
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
        state.current_place_rotation = Direction((state.current_place_rotation + 1) % DIR_COUNT);
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
      system_place_entity(
        state.store,
        state.player_id,
        state.tick_input,
        state.current_place_rotation
      );
      system_remove_entity(state.store, state.player_id, state.tick_input);
      system_pickup_item(state.store, state.player_id);
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
    } break;
    case MODE_EDITOR: {
      auto result = editor::update(state.editor, state.store, state.tick_input);
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
  state.frame = {};
  gather_input(state.frame_input);
  accumulate_input(state.tick_input, state.frame_input);
  ui_system_update(state.ui_system);

  switch (state.mode) {
    case MODE_GAME: {
      auto player_inv_hovered_slot = gui_player_inventory(
        state.ui_system,
        state.assets,
        state.frame_input,
        state.store,
        state.player_id
      );
      if (player_inv_hovered_slot) {
        state.frame.hovered_slot = player_inv_hovered_slot;
      }
      auto open_inventory_hovered_slot = gui_open_inventory(
        state.ui_system,
        state.assets,
        state.frame_input,
        state.store,
        state.player_id
      );
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
        state.ui_system,
        state.maintenance_minigame_texture,
        state.frame_input,
        state.assets,
        state.store,
        state.player_id,
        state.resource_message_queue,
        state.minutes
      );
      auto receiver_hovered_slot = gui_message_receiver(
        state.ui_system,
        state.maintenance_minigame_texture,
        state.frame_input,
        state.assets,
        state.store,
        state.player_id,
        state.resource_message_queue
      );
      if (receiver_hovered_slot) {
        state.frame.hovered_slot = receiver_hovered_slot;
      }

      auto assembler_hovered_slot = gui_assembler(
        state.ui_system,
        state.maintenance_minigame_texture,
        state.frame_input,
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
        editor::gui(state.editor, state.store, state.ui_system, state.frame_input, state.assets);
      if (result.save_requested) {
        save_state_to_file(state, DEFAULT_MAP_FILEPATH);
        std::println("saved state to '{}'", DEFAULT_MAP_FILEPATH);
      }
    } break;
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
              (grid_pos(state.frame_input.mouse_pos) * GRID_DIMS) + (GRID_DIMS / 2);
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
              case DIR_COUNT:
                ASSERT(false, "invalid rotation: DIR_COUNT");
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
              case DIR_COUNT:
                ASSERT(false, "invalid rotation: DIR_COUNT");
                break;
            }

            DrawLineV(main_start_pos.to_raylib(), main_end_pos.to_raylib(), ARROW_COLOR);
            DrawLineV(hands_start_pos.to_raylib(), right_end_pos.to_raylib(), ARROW_COLOR);
            DrawLineV(hands_start_pos.to_raylib(), left_end_pos.to_raylib(), ARROW_COLOR);
          }
        }
      }
    } break;
    case MODE_EDITOR: {
      editor::render(state.editor, state.store, state.assets);
    } break;
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
    if (!state.frame.hovered_slot) {
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
