#include "game.h"

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

    // TODO: not sure if this is supposed to happen before update_tick (1 tick ui latency i think)
    // but if it happens after,
    // the input is cleared in update_tick and sometimes clicked ui events dont register
    // i gather the input now at the start of it, so maybe it should happen here?
    // or maybe separating the input is better after all
    update_frame(state);

    while (accumulator >= DT) {
      update_tick(state, DT);
      accumulator -= DT;
    }

    render(state);
  }

  shutdown(state);

  return 0;
}
