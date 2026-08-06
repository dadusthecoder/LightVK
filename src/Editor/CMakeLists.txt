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

# Copy Assets to output directory
if(EXISTS "${CMAKE_SOURCE_DIR}/Assets")
    add_custom_command(TARGET LightVK_Editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets"
            "$<TARGET_FILE_DIR:LightVK_Editor>/Assets"
        COMMENT "Copying Assets → editor output"
    )
endif()