cmake_minimum_required(VERSION 3.17)
project(redgfx)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_program(
	GLSL_VALIDATOR
	glslangValidator
	DOC "Vulkan Shader Compiler (glslangValidator)"
	REQUIRED
)

set(REDGFX_SOURCE
    src/maths.h
    src/dependencies/volk.c
    src/dependencies/vk_mem_alloc.cpp
    src/dependencies/fast_obj.c
    src/main.c
    src/os.c
)

if (WIN32)
    set(SURFACE_DEFINITION VK_USE_PLATFORM_WIN32_KHR)
endif(WIN32)
if (UNIX)
    set(SURFACE_DEFINITION VK_USE_PLATFORM_XCB_KHR)
endif(UNIX)

set(SHADERS_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)

file(GLOB_RECURSE SHADER_SRC_FILES ${SHADERS_DIR}/*)
foreach(INPUT_SHADER ${SHADER_SRC_FILES})
    #get_filename_component(DIR ${INPUT_SHADER} DIRECTORY)
    get_filename_component(FILE_NAME ${INPUT_SHADER} NAME_WE)
    set(OUTPUT_BINARY "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${OUTPUT_BINARY}
        COMMAND ${GLSL_VALIDATOR} -V ${INPUT_SHADER} -o ${OUTPUT_BINARY}
        DEPENDS ${INPUT_SHADER})
    list(APPEND SPIRV_BINARY_FILES ${OUTPUT_BINARY})
endforeach(INPUT_SHADER)

add_custom_target(shaders
    DEPENDS ${SPIRV_BINARY_FILES})

add_executable(redgfx ${REDGFX_SOURCE})
add_dependencies(redgfx shaders)
target_compile_definitions(redgfx PUBLIC RED_DEBUG ${SURFACE_DEFINITION} _CRT_SECURE_NO_WARNINGS)
target_include_directories(redgfx PUBLIC ${Vulkan_INCLUDE_DIRS})
if(UNIX)
target_compile_options(redgfx PUBLIC -Wno-initializer-overrides -Werror=vla)
target_link_options(redgfx PUBLIC -rdynamic) # This linker flag gives more debugging information at run-time for logging purposes
target_link_libraries(redgfx PUBLIC ${Vulkan_LIBRARIES} glfw dl m)
endif(UNIX)
if (WIN32)
target_link_libraries(redgfx PUBLIC ${Vulkan_LIBRARIES} glfw)
endif(WIN32)

