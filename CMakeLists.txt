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

# TODO: can this also be used for gcc?
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
  -Wno-gnu-anonymous-struct -Wno-nested-anon-types
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
  ${mode_definitions}
  ${compiler_definitions}
  ${os_definitions}
)

set(flags
  "$<${compiler_clang}:${clang_flags_common}>"
  "$<$<AND:${compiler_clang},${mode_debug}>:${clang_flags_debug}>"
  "$<$<AND:${compiler_clang},$<OR:${mode_release},${mode_release_no_assert}>>:${clang_flags_release}>"
)

set(include_dirs src)

find_package(OpenGL REQUIRED)
include(vendor/vendor.cmake)

add_library(base STATIC
  src/base/array.h
  src/base/base.h
  src/base/map.h
  src/base/math.h src/base/math.cpp
  src/base/memory.h src/base/memory.cpp
  src/base/string.h src/base/string.cpp
  src/base/vertex.h src/base/vertex.cpp
)
target_compile_options(base PRIVATE ${flags})
target_compile_definitions(base PRIVATE ${definitions})

add_executable(game
  src/platform/platform.h src/platform/sdl3_gl.cpp
  src/platform/gl_functions.h src/platform/gl_functions.cpp

  src/game/assets.h src/game/assets.cpp
  src/game/camera.h src/game/camera.cpp
  src/game/entity.h src/game/entity.cpp
  src/game/image.h src/game/image.cpp
  src/game/renderer.h src/game/renderer.cpp
  src/game/game.cpp
)
target_include_directories(game PRIVATE ${include_dirs})
target_compile_options(game PRIVATE ${flags})
target_compile_definitions(game PRIVATE ${definitions})
target_link_libraries(game PRIVATE base SDL3::SDL3 OpenGL)

# NOTE: run a python script to check for missing error handling
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  find_package(Python3 REQUIRED)
  set(FIND_ERROR_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/find_error.py")

  add_custom_target(
        run_find_error ALL
        COMMAND ${Python3_EXECUTABLE} "${FIND_ERROR_SCRIPT}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Error handling check (find_error.py)"
        VERBATIM
    )

  add_dependencies(run_find_error game)
endif()

add_executable(tests src/tests/tests.cpp)
target_include_directories(tests PRIVATE ${include_dirs})
target_compile_options(tests PRIVATE ${flags})
target_compile_definitions(tests PRIVATE ${definitions})
target_link_libraries(tests base)
