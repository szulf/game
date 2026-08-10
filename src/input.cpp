#include "input.h"

void clear(Input& input) {
  auto mouse_pos  = input.mouse_pos;
  input           = {};
  input.mouse_pos = mouse_pos;
}

vec2 get_move_vector(const Input& input) {
  vec2 out{};
  if (action_state(input, Action::MOVE_UP).pressed()) {
    out.y -= 1;
  }
  if (action_state(input, Action::MOVE_DOWN).pressed()) {
    out.y += 1;
  }
  if (action_state(input, Action::MOVE_LEFT).pressed()) {
    out.x -= 1;
  }
  if (action_state(input, Action::MOVE_RIGHT).pressed()) {
    out.x += 1;
  }
  return out;
}
