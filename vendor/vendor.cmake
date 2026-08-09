set(vendored_include_dirs)

add_subdirectory(vendor/raylib EXCLUDE_FROM_ALL SYSTEM)

list(APPEND vendored_include_dirs vendor/json/)
