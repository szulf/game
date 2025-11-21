#include "time.hpp"

#ifdef BADTL_PLATFORM_SDL3
#  include "SDL3/SDL.h"
#endif

namespace btl {

namespace time {

u64 now() {
  return SDL_GetTicks();
}

void sleep_ms(u32 ms) {
  SDL_Delay(ms);
}

}

}
