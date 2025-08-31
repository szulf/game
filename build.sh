#!/bin/sh

mkdir -p build

clang \
    -std=c99 -g3 -O0 \
    -Wall -Wextra -Wconversion -Werror \
    -Wno-unused-function \
    -march=native \
    src/sdl3_game.c -o build/game \
    -lSDL3 -lGL \
    -DGAME_DEBUG -DGAME_OPENGL

    # TODO(szulf): compile SDL3 yourself to link statically
    # -static \
