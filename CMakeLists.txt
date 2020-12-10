cmake_minimum_required(VERSION 3.17)
project(redgfx)
set(CMAKE_C_COMPILER clang)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
set(REDGFX_SOURCE
    src/dependencies/volk.c
    src/main.c
    src/os.c
)
add_executable(redgfx ${REDGFX_SOURCE})
target_compile_definitions(redgfx PUBLIC RED_DEBUG VK_USE_PLATFORM_XCB_KHR GLFW_EXPOSE_NATIVE_X11)
target_compile_options(redgfx PUBLIC -Wno-initializer-overrides)
target_link_libraries(redgfx PUBLIC ${Vulkan_LIBRARIES} glfw dl m)
target_link_options(redgfx PUBLIC -rdynamic)
