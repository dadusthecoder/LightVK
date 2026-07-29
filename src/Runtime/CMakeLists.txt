file(GLOB_RECURSE RUNTIME_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_executable(LightVK_Runtime
    ${RUNTIME_SOURCES}
)

add_dependencies(LightVK_Runtime compile_shaders)

target_link_libraries(LightVK_Runtime
    PRIVATE
        LightVKEngine
)

# Shaders compile directly to the Bin directory now
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET LightVK_Runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:LightVK_Runtime>/assets"
        COMMENT "Copying assets → runtime output"
    )
endif()
