cmake_minimum_required(VERSION 3.20)

# 1. Standard Toolchain Configuration
set(CMAKE_TOOLCHAIN_FILE
    "D:/dev/cpp/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING ""
)
set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")

project(LightVK VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bin")

# 2. Pure vcpkg Dependency Discovery
find_package(Vulkan REQUIRED)
find_package(volk CONFIG REQUIRED) # <-- Added: Tell vcpkg to find Volk configuration maps
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fastgltf CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)

find_program(GLSLC glslc REQUIRED)

# 3. Shader Compilation Logic
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_BINARY_DIR "${CMAKE_BINARY_DIR}/Bin/shaders")

file(GLOB SHADER_SOURCES
    "${SHADER_SOURCE_DIR}/*.vert"
    "${SHADER_SOURCE_DIR}/*.frag"
    "${SHADER_SOURCE_DIR}/*.comp"
)

set(COMPILED_SHADERS)
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT "${SHADER_BINARY_DIR}/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
        COMMAND ${GLSLC} --target-env=vulkan1.3 --target-spv=spv1.6 ${SHADER} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER_NAME}"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${SHADER_OUTPUT})
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${COMPILED_SHADERS})

# 4. Route execution down to targets sequentially
add_subdirectory(src/Engine)
add_subdirectory(src/Editor)
add_subdirectory(src/Runtime)
