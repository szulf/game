#pragma once

#include "core.h"
#include "math.h"

enum GKey {
  GKEY_A,
  GKEY_B,
  GKEY_C,
  GKEY_D,
  GKEY_E,
  GKEY_F,
  GKEY_G,
  GKEY_H,
  GKEY_I,
  GKEY_J,
  GKEY_K,
  GKEY_L,
  GKEY_M,
  GKEY_N,
  GKEY_O,
  GKEY_P,
  GKEY_Q,
  GKEY_R,
  GKEY_S,
  GKEY_T,
  GKEY_U,
  GKEY_V,
  GKEY_W,
  GKEY_X,
  GKEY_Y,
  GKEY_Z,
  GKEY_0,
  GKEY_1,
  GKEY_2,
  GKEY_3,
  GKEY_4,
  GKEY_5,
  GKEY_6,
  GKEY_7,
  GKEY_8,
  GKEY_9,
  GKEY_F1,
  GKEY_F2,
  GKEY_F3,
  GKEY_F4,
  GKEY_F5,
  GKEY_F6,
  GKEY_F7,
  GKEY_F8,
  GKEY_F9,
  GKEY_F10,
  GKEY_F11,
  GKEY_F12,
  GKEY_SPACE,
  GKEY_LSHIFT,
  GKEY_TAB,
  GKEY_ESCAPE,
  GKEY_COUNT,
};

struct KeyState {
  u32 transition_count{};
  bool down{};

  inline bool pressed() const {
    return down && transition_count != 0;
  }
};

struct Input {
  std::array<KeyState, GKEY_COUNT> keys{};

  vec2 mouse_pos{};
  KeyState lmb{};
  KeyState rmb{};
  i32 mouse_scroll{};
};

void gather_input(Input& input);
void accumulate_input(Input& to, const Input& from);
void clear(Input& input);

enum Action {
  ACTION_MOVE_UP,
  ACTION_MOVE_LEFT,
  ACTION_MOVE_DOWN,
  ACTION_MOVE_RIGHT,
  ACTION_INTERACT,
  ACTION_CLOSE_INV,
  ACTION_ROTATE,

  ACTION_SERIALIZE,
  ACTION_DESERIALIZE,

  ACTION_TOGGLE_DEBUG_RENDERING,
  ACTION_TOGGLE_EDITOR_MODE,

  ACTION_COUNT,
};

static constexpr std::array<GKey, ACTION_COUNT> KEYMAP = []() {
  std::array<GKey, ACTION_COUNT> map{};
  map[ACTION_MOVE_UP]    = GKEY_W;
  map[ACTION_MOVE_LEFT]  = GKEY_A;
  map[ACTION_MOVE_DOWN]  = GKEY_S;
  map[ACTION_MOVE_RIGHT] = GKEY_D;
  map[ACTION_INTERACT]   = GKEY_E;
  map[ACTION_CLOSE_INV]  = GKEY_ESCAPE;
  map[ACTION_ROTATE]     = GKEY_R;

  map[ACTION_SERIALIZE]   = GKEY_F1;
  map[ACTION_DESERIALIZE] = GKEY_F2;

  map[ACTION_TOGGLE_DEBUG_RENDERING] = GKEY_F3;
  map[ACTION_TOGGLE_EDITOR_MODE]     = GKEY_F4;
  return map;
}();

inline const KeyState& action_state(const Input& input, Action action) {
  return input.keys[KEYMAP[action]];
}

vec2 get_move_vector(const Input& input);
