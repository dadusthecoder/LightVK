file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(LightVKEngine SHARED
    ${CORE_SOURCES}
)


set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/ThirdParty/imgui)
set(IMGUI_NODE_EDITOR_DIR ${CMAKE_SOURCE_DIR}/ThirdParty/imgui-node-editor)
set(STB_IMAGE_DIR ${IMGUI_NODE_EDITOR_DIR}/external/stb_image)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_NODE_EDITOR_DIR}/crude_json.cpp
    ${IMGUI_NODE_EDITOR_DIR}/imgui_canvas.cpp
    ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor.cpp
    ${IMGUI_NODE_EDITOR_DIR}/imgui_node_editor_api.cpp
)

target_sources(LightVKEngine PRIVATE ${IMGUI_SOURCES})

target_include_directories(LightVKEngine
    PUBLIC
    "${CMAKE_SOURCE_DIR}/src"
    "D:/Vulkan/Include/Volk"
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUI_NODE_EDITOR_DIR}
    ${STB_IMAGE_DIR}
)

find_library(VOLK_LIB NAMES volk PATHS D:/Vulkan/Lib REQUIRED)

target_link_libraries(LightVKEngine
    PUBLIC
    Vulkan::Headers 
    ${VOLK_LIB}
    glfw
    glm::glm
    spdlog::spdlog
    fastgltf::fastgltf
    EnTT::EnTT
)

target_compile_definitions(LightVKEngine
    PRIVATE
    LIGHTVK_CORE_BUILD
    PUBLIC
    IMGUI_IMPL_VULKAN_USE_VOLK
    VK_NO_PROTOTYPES 
    VK_ENABLE_BETA_EXTENSIONS 
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS
    GLM_FORCE_PURE
)

target_compile_options(LightVKEngine PRIVATE "-Wno-nullability-completeness")
target_link_options(LightVKEngine PRIVATE "-Xlinker" "/ignore:4099")



