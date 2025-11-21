#include "threads.hpp"

#include "utils.hpp"

namespace btl {

#ifdef BADTL_PLATFORM_SDL3

Thread Thread::make(ThreadFunction fn, void* args) {
  Thread t = {};
  t.platform_data.thread = SDL_CreateThread(fn, nullptr, args);
  ASSERT(t.platform_data.thread != nullptr, "couldnt create thread");
  return t;
}

void Thread::detach() {
  SDL_DetachThread(platform_data.thread);
}

void Thread::wait() {
  SDL_WaitThread(platform_data.thread, nullptr);
}

Mutex Mutex::make() {
  Mutex m = {};
  m.platform_data.mutex = SDL_CreateMutex();
  return m;
}

#endif

}
