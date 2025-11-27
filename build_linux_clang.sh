#!/bin/sh

mkdir -p build

flags="
  -g3 -O0 -Weverything -Werror -pedantic
  -fno-rtti -fno-exceptions -fPIC
  -Wno-c++98-compat -Wno-c++98-compat-pedantic
  -Wno-padded -Wno-switch-default -Wno-unsafe-buffer-usage
  -Wno-unreachable-code-break -Wno-cast-function-type-strict
  -Wno-weak-vtables -Wno-exit-time-destructors -Wno-ctad-maybe-unsupported
  -Wno-unused-macros -Wno-missing-noreturn -Wno-covered-switch-default
  -Wno-undefined-func-template -Wno-missing-designated-field-initializers
  -Wno-global-constructors -Wno-unused-function -Wno-unused-template
  -Wno-format-nonliteral -Wno-shadow-uncaptured-local
  -Wno-old-style-cast -Wno-missing-prototypes -Wno-double-promotion
  -nostdlib++ -nostdinc++"

options="-DPLATFORM_LINUX -DCOMPILER_CLANG -DMODE_DEBUG -DASSERTIONS -DRENDERER_OPENGL"

clang++ \
  $flags \
  $options \
  -shared -fPIC -fvisibility=hidden \
  -o build/libgame.so \
  src/game.cpp

clang++ $flags $options \
    -o build/game \
    src/sdl3_gl.cpp \
    -lSDL3 -lGL
