cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(game
    VERSION 1.0
    LANGUAGES CXX
)

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES
      "Debug;Release;ReleaseNoAssert"
      CACHE STRING "" FORCE
  )
elseif(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# TODO(szulf): can this also be used for gcc?
set(clang_flags_common
  -Weverything -Werror -pedantic
  -fno-rtti -fno-exceptions
  -Wno-c++98-compat -Wno-c++98-compat-pedantic
  -Wno-padded -Wno-switch-default -Wno-unsafe-buffer-usage
  -Wno-unreachable-code-break -Wno-cast-function-type-strict
  -Wno-weak-vtables -Wno-exit-time-destructors -Wno-ctad-maybe-unsupported
  -Wno-unused-macros -Wno-missing-noreturn -Wno-covered-switch-default
  -Wno-undefined-func-template -Wno-missing-designated-field-initializers
  -Wno-global-constructors -Wno-unused-function -Wno-unused-template
  -Wno-format-nonliteral -Wno-shadow-uncaptured-local
  -Wno-old-style-cast -Wno-missing-prototypes -Wno-double-promotion
  -Wno-documentation-unknown-command -Wno-reserved-macro-identifier
  -nostdlib++ -nostdinc++
)
set(clang_flags_debug
  -g3 -O0
)
set(clang_flags_release
  -O3
)

set(compiler_clang "$<CXX_COMPILER_ID:Clang>")
set(compiler_definitions
  "$<${compiler_clang}:COMPILER_CLANG>"
)

set(os_linux "$<PLATFORM_ID:Linux>")
set(os_definitions
  "$<${os_linux}:OS_LINUX>"
)

set(mode_debug "$<CONFIG:Debug>")
set(mode_release "$<CONFIG:Release>")
set(mode_release_no_assert "$<CONFIG:ReleaseNoAssert>")
set(mode_definitions
  "$<${mode_debug}:MODE_DEBUG>" "$<${mode_debug}:ASSERTIONS>"
  "$<${mode_release}:ASSERTIONS>"
)

set(definitions
  # NOTE(szulf): for now(probably forever) only opengl
  RENDERER_OPENGL
  ${mode_definitions}
  ${compiler_definitions}
  ${os_definitions}
)

set(flags
  "$<${compiler_clang}:${clang_flags_common}>"
  "$<$<AND:${compiler_clang},${mode_debug}>:${clang_flags_debug}>"
  "$<$<AND:${compiler_clang},$<OR:${mode_release},${mode_release_no_assert}>>:${clang_flags_release}>"
)

# NOTE(szulf): using unity build so thats the reason for just one source file
add_library(game SHARED src/game.cpp)
target_compile_options(game PRIVATE ${flags} -fPIC -fvisibility=hidden)
target_compile_definitions(game PRIVATE ${definitions})

find_package(OpenGL REQUIRED)
include(vendor/vendor.cmake)

# NOTE(szulf): using unity build so thats the reason for just one source file
add_executable(game_sdl3 src/sdl3_gl.cpp)
target_compile_options(game_sdl3 PRIVATE ${flags})
target_compile_definitions(game_sdl3 PRIVATE ${definitions})
target_link_libraries(game_sdl3 PRIVATE SDL3::SDL3 OpenGL)
