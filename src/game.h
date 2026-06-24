#ifndef GAME_H
#define GAME_H

struct Input {
  ivec2 mouse_pos{};
  bool lmb_pressed{};
  bool rmb_pressed{};

  ivec2 move{};
  bool interact{};
  bool close_inv{};
  bool rotate{};

  bool toggle_debug{};
};

void clear(Input& input);

// NOTE: would be better if this could just be the item entity,
// but then i dont have a way to say im hovering over an empty item slot
struct ItemSlotIdx {
  EntityId entity{};
  u32 slot_idx{};
};

struct FrameData {
  ItemSlotIdx hovered_slot{};
  std::vector<ui::Command> ui_cmds{};
};

void clear(FrameData& frame);

struct State {
  Input input{};
  FrameData frame{};

  // TODO: should reset when changing the player hand item
  Rotation current_place_rotation{};

  // NOTE: there is only one player
  EntityId player_id{};
  EntityStore store{};

  bool debug{};
};

static constexpr ivec2 WINDOW_DIMS = {1280, 720};
static constexpr ivec2 GRID_DIMS   = {32, 32};

void init(State& state);
void gather_input(State& state);
void update_tick(State& state, f32 dt);
void update_frame(State& state);
void render(State& state);
void shutdown(State& state);

#endif
