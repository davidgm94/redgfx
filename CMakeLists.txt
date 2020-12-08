project(gfx)
set(CMAKE_C_COMPILER clang)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(GFX_SOURCE
    src/main.c
)
add_executable(gfx ${GFX_SOURCE})

