#!/bin/sh

mkdir -p build

clang++ \
    -std=c++17 -g3 -O0 \
    -Wall -Wextra -Wconversion -Werror \
    -march=native -fno-rtti -fno-exceptions \
    src/sdl3_game.cpp -o build/game \
    -lSDL3 -lGL \
    -DGAME_DEBUG -DGAME_OPENGL

    # TODO(szulf): compile SDL3 yourself to link statically
    # -static \
