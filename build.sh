#!/bin/sh

mkdir -p build

    # src/SDL_game.cpp -o build/game \
clang++ \
    -std=c++11 -g3 -O0 \
    -Wall -Wextra -Wconversion \
    -march=native -fno-rtti -fno-exceptions \
    src/sdl3_game.cpp -o build/game \
    -lSDL3 -lGL \
    -DGAME_INTERNAL

    # TODO(szulf): compile SDL3 yourself to link statically
    # -static \
