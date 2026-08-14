#include "input.h"

i32 gkey_to_raylib_key(GKey key) {
  switch (key) {
    case GKEY_A:
      return KEY_A;
    case GKEY_B:
      return KEY_B;
    case GKEY_C:
      return KEY_C;
    case GKEY_D:
      return KEY_D;
    case GKEY_E:
      return KEY_E;
    case GKEY_F:
      return KEY_F;
    case GKEY_G:
      return KEY_G;
    case GKEY_H:
      return KEY_H;
    case GKEY_I:
      return KEY_I;
    case GKEY_J:
      return KEY_J;
    case GKEY_K:
      return KEY_K;
    case GKEY_L:
      return KEY_L;
    case GKEY_M:
      return KEY_M;
    case GKEY_N:
      return KEY_N;
    case GKEY_O:
      return KEY_O;
    case GKEY_P:
      return KEY_P;
    case GKEY_Q:
      return KEY_Q;
    case GKEY_R:
      return KEY_R;
    case GKEY_S:
      return KEY_S;
    case GKEY_T:
      return KEY_T;
    case GKEY_U:
      return KEY_U;
    case GKEY_V:
      return KEY_V;
    case GKEY_W:
      return KEY_W;
    case GKEY_X:
      return KEY_X;
    case GKEY_Y:
      return KEY_Y;
    case GKEY_Z:
      return KEY_Z;
    case GKEY_0:
      return KEY_ZERO;
    case GKEY_1:
      return KEY_ONE;
    case GKEY_2:
      return KEY_TWO;
    case GKEY_3:
      return KEY_THREE;
    case GKEY_4:
      return KEY_FOUR;
    case GKEY_5:
      return KEY_FIVE;
    case GKEY_6:
      return KEY_SIX;
    case GKEY_7:
      return KEY_SEVEN;
    case GKEY_8:
      return KEY_EIGHT;
    case GKEY_9:
      return KEY_NINE;
    case GKEY_F1:
      return KEY_F1;
    case GKEY_F2:
      return KEY_F2;
    case GKEY_F3:
      return KEY_F3;
    case GKEY_F4:
      return KEY_F4;
    case GKEY_F5:
      return KEY_F5;
    case GKEY_F6:
      return KEY_F6;
    case GKEY_F7:
      return KEY_F7;
    case GKEY_F8:
      return KEY_F8;
    case GKEY_F9:
      return KEY_F9;
    case GKEY_F10:
      return KEY_F10;
    case GKEY_F11:
      return KEY_F11;
    case GKEY_F12:
      return KEY_F12;
    case GKEY_SPACE:
      return KEY_SPACE;
    case GKEY_LSHIFT:
      return KEY_LEFT_SHIFT;
    case GKEY_TAB:
      return KEY_TAB;
    case GKEY_ESCAPE:
      return KEY_ESCAPE;
    case GKEY_COUNT:
      break;
  }

  ASSERT(false, "invalid key: %d", i32(key));
}

KeyState get_key_state(GKey key) {
  auto raylib_key = gkey_to_raylib_key(key);
  KeyState state{};
  state.down = IsKeyDown(raylib_key);
  if (IsKeyPressed(raylib_key)) {
    state.transition_count += 1;
  }
  // if (IsKeyReleased(raylib_key)) {
  //   state.transition_count += 1;
  // }
  return state;
}

// TODO: not accounting for key release in transition_count
// TODO: not sure whether the tick_input is fully correct
void gather_input(Input& input) {
  clear(input);

  input.mouse_pos    = vec2::from_raylib(GetMousePosition());
  input.mouse_scroll = GetMouseWheelMove();

  input.lmb.down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    input.lmb.transition_count += 1;
  }
  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    input.lmb.transition_count += 1;
  }

  input.rmb.down = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    input.rmb.transition_count += 1;
  }
  if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
    input.rmb.transition_count += 1;
  }

  for (u32 i = 0; i < input.keys.size(); ++i) {
    auto key        = GKey(i);
    input.keys[key] = get_key_state(key);
  }
}

void accumulate_input(Input& to, const Input& from) {
  to.mouse_pos = from.mouse_pos;
  to.mouse_scroll += from.mouse_scroll;

  to.lmb.down = from.lmb.down;
  to.lmb.transition_count += from.lmb.transition_count;

  to.rmb.down = from.rmb.down;
  to.rmb.transition_count += from.rmb.transition_count;

  for (u32 i = 0; i < from.keys.size(); ++i) {
    auto key         = GKey(i);
    auto& to_state   = to.keys[key];
    auto& from_state = from.keys[key];
    to_state.down    = from_state.down;
    to_state.transition_count += from_state.transition_count;
  }
}

void clear(Input& input) {
  auto mouse_pos  = input.mouse_pos;
  input           = {};
  input.mouse_pos = mouse_pos;
}

vec2 get_move_vector(const Input& input) {
  vec2 out{};
  if (action_state(input, ACTION_MOVE_UP).pressed()) {
    out.y -= 1;
  }
  if (action_state(input, ACTION_MOVE_DOWN).pressed()) {
    out.y += 1;
  }
  if (action_state(input, ACTION_MOVE_LEFT).pressed()) {
    out.x -= 1;
  }
  if (action_state(input, ACTION_MOVE_RIGHT).pressed()) {
    out.x += 1;
  }
  return out;
}
