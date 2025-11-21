#ifndef THREADS_HPP
#define THREADS_HPP

#ifdef BADTL_PLATFORM_SDL3
#  include <SDL3/SDL.h>
#endif

#include "types.hpp"

namespace btl {

using ThreadFunction = int (*)(void*);

#ifdef BADTL_PLATFORM_SDL3

struct Thread {
  struct PlatformData {
    SDL_Thread* thread;
  };

  static Thread make(ThreadFunction fn, void* args);
  void wait();
  void detach();

  PlatformData platform_data;
};

struct Mutex {
  struct PlatformData {
    SDL_Mutex* mutex;
  };

  static Mutex make();

  inline void lock();
  inline void unlock();

  PlatformData platform_data;
};

struct AtomicBool {
  struct PlatformData {
    SDL_AtomicU32 atomic_u32;
  };

  inline void set(bool value);
  inline bool get();

  PlatformData platform_data;
};

#endif

}

namespace btl {

#ifdef BADTL_PLATFORM_SDL3

inline void Mutex::lock() {
  SDL_LockMutex(platform_data.mutex);
}

inline void Mutex::unlock() {
  SDL_UnlockMutex(platform_data.mutex);
}

inline void AtomicBool::set(bool value) {
  SDL_SetAtomicU32(&platform_data.atomic_u32, static_cast<btl::u32>(value));
}

inline bool AtomicBool::get() {
  return SDL_GetAtomicU32(&platform_data.atomic_u32) == static_cast<btl::u32>(true);
}

#endif

}

#endif
