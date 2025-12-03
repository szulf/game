# This assumes the SDL source is available in vendored/SDL
set(SDL_SHARED false)
set(SDL_STATIC true)
add_subdirectory(vendor/sdl3 EXCLUDE_FROM_ALL)
