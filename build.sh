#!/bin/sh

mkdir -p build

clang++ \
    -std=c++20 -g3 -O0 \
    -Wall -Wextra -Wconversion -Werror \
    -Wno-unused-function -Wvla \
    -fno-builtin-sin -fno-builtin-cos -fno-builtin-sqrt -fno-builtin-mod -fno-builtin-acos -fno-builtin-tan -fno-builtin-fmax -fno-builtin-fmin \
    -march=native \
    src/sdl3_game.cpp -o build/game \
    -lSDL3 -lGL \
    -DGAME_DEBUG -DGAME_OPENGL

    # TODO(szulf): compile SDL3 yourself to link statically
    # -static \
