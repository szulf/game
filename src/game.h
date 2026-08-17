#pragma once

#include <string_view>

#include "core.h"
#include "assets.h"
#include "input.h"
#include "ui.h"
#include "entity.h"
#include "editor.h"

// TODO: when deserializing the std::vector's may get a wrong size,
// if i serialized them with one and then i change it to something else,
// the old one will still remain

struct FrameData {
  ItemSlotIdx hovered_slot{};
  vec2 window_dims{};
  vec2 mouse_world_pos{};
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
  Camera2D camera{};

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

  Editor editor{};
};

void init(State& state);
void update_tick(State& state, f32 dt);
void update_frame(State& state);
void render(State& state);
void shutdown(State&);
