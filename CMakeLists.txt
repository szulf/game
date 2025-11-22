cmake_minimum_required(VERSION 3.15)

# TODO(szulf): split this into file per library/executable
# TODO(szulf): instead of FetchContent use vendor/

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(game
    VERSION 1.0
    LANGUAGES CXX
)

set(COMPILE_FLAGS
  -g3 -O0 -Weverything -Werror -pedantic
  -fno-rtti -fno-exceptions
  -Wno-c++98-compat -Wno-c++98-compat-pedantic
  -Wno-padded -Wno-switch-default -Wno-unsafe-buffer-usage
  -Wno-unreachable-code-break -Wno-cast-function-type-strict
  -Wno-weak-vtables -Wno-exit-time-destructors -Wno-ctad-maybe-unsupported
  -Wno-unused-macros -Wno-missing-noreturn -Wno-covered-switch-default
  -Wno-undefined-func-template -Wno-missing-designated-field-initializers
  -Wno-global-constructors -Wno-unused-function -Wno-unused-template
  -Wno-format-nonliteral -Wno-shadow-uncaptured-local
  -nostdlib++ -nostdinc++
)

include(FetchContent)
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL
  GIT_TAG badbf8da4ee72b3ef599c721ffc9899e8d7c8d90
)
FetchContent_MakeAvailable(SDL3)

add_library(badtl STATIC
  badtl/allocator.cpp badtl/allocator.hpp
  badtl/array.hpp
  badtl/files.cpp  badtl/files.hpp
  badtl/function.hpp
  badtl/list.hpp
  badtl/map.hpp
  badtl/mat4.cpp badtl/mat4.hpp
  badtl/math.hpp
  badtl/memory.hpp
  badtl/print.hpp
  badtl/ptr.hpp
  badtl/result.hpp
  badtl/string.cpp badtl/string.hpp
  badtl/time.cpp badtl/time.hpp
  badtl/types.hpp
  badtl/utils.hpp
  badtl/vec2.cpp badtl/vec2.hpp
  badtl/vec3.cpp badtl/vec3.hpp
)
target_compile_options(badtl PRIVATE ${COMPILE_FLAGS})
target_link_libraries(badtl PUBLIC SDL3)
target_compile_definitions(badtl PUBLIC BADTL_PLATFORM_SDL3)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_definitions(badtl PUBLIC BADTL_COMPILER_CLANG)
endif()

message(${CMAKE_CXX_COMPILER_ID})

find_package(OpenGL REQUIRED)

add_library(engine
  engine/asset_manager.cpp engine/asset_manager.hpp
  engine/engine.hpp
  engine/event.hpp
  engine/image.cpp engine/image.hpp
  engine/key.hpp

  engine/renderer/camera.cpp engine/renderer/camera.hpp
  engine/renderer/gl_functions.cpp engine/renderer/gl_functions.hpp
  engine/renderer/material.hpp
  engine/renderer/mesh.cpp engine/renderer/mesh.hpp
  engine/renderer/model.cpp engine/renderer/model.hpp
  engine/renderer/renderer.cpp engine/renderer/renderer.hpp
  engine/renderer/scene.cpp engine/renderer/scene.hpp
  engine/renderer/shaders.cpp engine/renderer/shaders.hpp
  engine/renderer/texture.cpp engine/renderer/texture.hpp
  engine/renderer/vertex.cpp engine/renderer/vertex.hpp
)
# TODO(szulf): is the . the correct directory to specify,
# can i somehow be more specific?
target_include_directories(engine PRIVATE .)
target_compile_options(engine PRIVATE ${COMPILE_FLAGS})
target_link_libraries(engine PUBLIC badtl SDL3 OpenGL)

add_executable(game
    game/main.cpp
)
target_compile_options(game PRIVATE ${COMPILE_FLAGS})

add_compile_definitions(GAME_OPENGL)
add_compile_definitions(GAME_DEBUG)
add_compile_definitions(GAME_SDL3)


target_include_directories(game PRIVATE .)
target_link_libraries(game PRIVATE engine)
