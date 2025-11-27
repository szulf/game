#include "time.h"

u64 time_now() {
  return SDL_GetTicks();
}

void sleep_ms(u32 ms) {
  SDL_Delay(ms);
}
