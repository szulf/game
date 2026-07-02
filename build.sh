#!/bin/sh

cd vendor/raylib/src
make PLATFORM=PLATFORM_DESKTOP
cd ../../../

clang++ \
    -std=c++23 \
    -o main \
    src/main.cpp \
    -g3 -Wall -Wextra -Werror \
    -Wno-missing-designated-field-initializers \
    -lraylib -Lvendor/raylib/src -isystem vendor/raylib/src -lX11
