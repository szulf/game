#ifndef GAME_H
#define GAME_H

enum class Direction {
  Up,
  Right,
  Down,
  Left,
  Count,
};

// NOTE: components start

using Position = ivec2;

struct RenderRect {
  Color color{};
  ivec2 dims{};
};

inline bool operator==(const RenderRect& a, const RenderRect& b) {
  return a.color == b.color && a.dims == b.dims;
}

struct Player {
  i32 interaction_radius{};
  ecs::Entity open_inventory{};
  ecs::Entity hand{};
};

using Rotation = Direction;

// NOTE: storing the component map to properly restore components the entity had
struct Item {
  u32 count{};
  ecs::ComponentMap component_map{};
};

static constexpr u32 ITEM_MAX_COUNT = 100;

using Inventory = std::vector<ecs::Entity>;

struct Solid {};

struct Breakable {};

// NOTE: components end

// NOTE: events start

struct PlayerCollided {
  ecs::Entity entity{};
};

// NOTE: events end

// TODO: not sure if right and left degrees are correct
f32 rotation_degrees(Rotation rotation) {
  switch (rotation) {
    case Rotation::Up:
      return 0;
    case Rotation::Down:
      return 180;
    case Rotation::Right:
      return 90;
    case Rotation::Left:
      return 270;
    case Rotation::Count:
      break;
  }
  ASSERT(false);
}

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
  ecs::Entity entity;
  u32 slot_idx;
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
  ecs::Entity player_entity{};
  ecs::World world{};

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
