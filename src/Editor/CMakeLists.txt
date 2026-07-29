file(GLOB_RECURSE EDITOR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_executable(LightVK_Editor
    ${EDITOR_SOURCES}
)

add_dependencies(LightVK_Editor compile_shaders)

target_link_libraries(LightVK_Editor
    PRIVATE
        LightVKEngine       
)

# Shaders compile directly to the Bin directory now
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET LightVK_Editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:LightVK_Editor>/assets"
        COMMENT "Copying assets → editor output"
    )
endif()