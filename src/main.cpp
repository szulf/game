#include "game.cpp"

int main() {
  State state = {};
  init(state);

  f64 current_time = GetTime();
  f64 accumulator  = 0;

  // TODO: replace WindowShouldClose() (a raylib function) to something else?
  while (!WindowShouldClose()) {
    f64 new_time   = GetTime();
    f64 frame_time = new_time - current_time;
    current_time   = new_time;
    accumulator += frame_time;

    gather_input(state);

    while (accumulator >= DT) {
      update_tick(state, DT);
      accumulator -= DT;
    }

    update_frame(state);
    render(state);
  }

  shutdown(state);

  return 0;
}
