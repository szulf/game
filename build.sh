#!/bin/sh

mkdir -p build

gcc \
    -std=c99 -g3 -O0 \
    -pedantic \
    -Wall -Wextra -Wconversion -Werror \
    -Wno-unused-function -Wvla \
    -fno-builtin-sin -fno-builtin-cos -fno-builtin-sqrt -fno-builtin-mod -fno-builtin-acos -fno-builtin-tan -fno-builtin-fmax -fno-builtin-fmin \
    -march=native \
    src/sdl3_game.c -o build/game \
    -lSDL3 -lGL \
    -DGAME_DEBUG -DGAME_SDL -DGAME_OPENGL

    # TODO(szulf): compile SDL3 yourself to link statically
    # -static \
