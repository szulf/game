cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(game
    VERSION 1.0
    LANGUAGES CXX
)

include(FetchContent)
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL
  GIT_TAG badbf8da4ee72b3ef599c721ffc9899e8d7c8d90
)
FetchContent_MakeAvailable(SDL3)

find_package(OpenGL REQUIRED)

# TODO(szulf): move engine to its own cmakelists file
add_executable(game
    game/main.cpp
    engine/sdl3_gl_engine.cpp engine/engine.hpp
)

target_compile_options(game
   PRIVATE -Werror -Wall -Wextra -Wconversion -pedantic)

target_include_directories(game PRIVATE engine)
target_link_libraries(game PRIVATE SDL3 OpenGL)
