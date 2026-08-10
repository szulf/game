#pragma once

#include "core.h"
#include "utils.h"

enum class Key {
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  ZERO,
  ONE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  SEVEN,
  EIGHT,
  NINE,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  SPACE,
  LSHIFT,
  TAB,
  ESCAPE,
  COUNT,
};

struct KeyState {
  u32 transition_count{};
  bool down{};

  inline bool pressed() const {
    return down && transition_count != 0;
  }
};

struct Input {
  EnumArray<KeyState, Key> keys{};

  vec2 mouse_pos{};
  KeyState lmb{};
  KeyState rmb{};
  i32 mouse_scroll{};
};

void clear(Input& input);

enum class Action {
  MOVE_UP,
  MOVE_LEFT,
  MOVE_DOWN,
  MOVE_RIGHT,
  INTERACT,
  CLOSE_INV,
  ROTATE,

  SERIALIZE,
  DESERIALIZE,

  TOGGLE_DEBUG_RENDERING,
  TOGGLE_EDITOR_MODE,

  COUNT,
};

static constexpr EnumArray<Key, Action> KEYMAP = []() {
  EnumArray<Key, Action> map{};
  map[Action::MOVE_UP]    = Key::W;
  map[Action::MOVE_LEFT]  = Key::A;
  map[Action::MOVE_DOWN]  = Key::S;
  map[Action::MOVE_RIGHT] = Key::D;
  map[Action::INTERACT]   = Key::E;
  map[Action::CLOSE_INV]  = Key::ESCAPE;
  map[Action::ROTATE]     = Key::R;

  map[Action::SERIALIZE]   = Key::F1;
  map[Action::DESERIALIZE] = Key::F2;

  map[Action::TOGGLE_DEBUG_RENDERING] = Key::F3;
  map[Action::TOGGLE_EDITOR_MODE]     = Key::F4;
  return map;
}();

inline const KeyState& action_state(const Input& input, Action action) {
  return input.keys[KEYMAP[action]];
}

vec2 get_move_vector(const Input& input);
