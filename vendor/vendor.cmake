# This assumes the SDL source is available in vendored/SDL
set(SDL_SHARED false)
set(SDL_STATIC true)
add_subdirectory(vendor/sdl3 EXCLUDE_FROM_ALL)

add_library(stb STATIC
  vendor/stb/image.h vendor/stb/image.cpp
)
