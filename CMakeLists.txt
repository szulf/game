cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(game
    VERSION 0.1
    LANGUAGES CXX
)

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES
      "Debug;Release"
      CACHE STRING "" FORCE
  )
elseif(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
endif()

# TODO: check for the right compiler here?
set(CMAKE_CXX_FLAGS "-fno-exceptions")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Werror -Wno-missing-designated-field-initializers")

set(definitions
  "COMPILER_CLANG=$<CXX_COMPILER_ID:Clang>"
  "COMPILER_GCC=$<CXX_COMPILER_ID:GNU>"
  "COMPILER_MSVC=$<CXX_COMPILER_ID:MSVC>"

  "OS_LINUX=$<PLATFORM_ID:Linux>"
  "OS_WINDOWS=$<PLATFORM_ID:Windows>"
  "OS_MACOS=$<PLATFORM_ID:Darwin>"

  "MODE_DEBUG=$<CONFIG:Debug>"
  "MODE_RELEASE=$<CONFIG:Release>"
)

include(vendor/vendor.cmake)

# TODO: actually wire this up
# add_compile_options(-fsanitize=address,undefined)
# add_link_options(-fsanitize=address,undefined)

add_executable(game
  src/main.cpp
)
target_include_directories(game PRIVATE ${vendored_include_dirs} SYSTEM)
target_compile_definitions(game PRIVATE ${definitions})
target_link_libraries(game PRIVATE raylib)
