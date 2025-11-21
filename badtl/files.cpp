#include "files.hpp"

#ifdef BADTL_PLATFORM_SDL3
#  include "SDL3/SDL.h"
#endif

#include "utils.hpp"

namespace btl {

#ifdef BADTL_PLATFORM_SDL3

Ptr<void> readFile(const String& path, Allocator& allocator) {
  auto scratch_arena = btl::ScratchArena::get();
  defer(scratch_arena.release());
  return readFile(path.cString(scratch_arena.allocator), allocator);
}

Ptr<void> readFile(const char* filepath, Allocator& allocator) {
  SDL_Storage* storage = SDL_OpenFileStorage(nullptr);
  defer(SDL_CloseStorage(storage));

  u64 file_size;
  if (!SDL_GetStorageFileSize(storage, filepath, &file_size)) {
    return {};
  }
  void* file = allocator.alloc(file_size);
  if (!SDL_ReadStorageFile(storage, filepath, file, file_size)) {
    return {};
  }
  return {file, file_size};
}

#endif

}
