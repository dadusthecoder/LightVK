file(GLOB_RECURSE EDITOR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

file(GLOB_RECURSE EDITOR_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.h"
)

list(REMOVE_ITEM EDITOR_SOURCES ${EDITOR_TEST_SOURCES})

add_executable(LightVK_Editor
    ${EDITOR_SOURCES}
)

target_sources(LightVK_Editor
    PRIVATE
        $<$<NOT:$<CONFIG:Release>>:${EDITOR_TEST_SOURCES}>
)

add_dependencies(LightVK_Editor compile_shaders)

target_link_libraries(LightVK_Editor
    PRIVATE
        LightVKEngine       
)

# Editor tests are included in non-Release editor builds only.
target_compile_definitions(LightVK_Editor
    PRIVATE
        $<$<NOT:$<CONFIG:Release>>:LIGHTVK_EDITOR_TESTS>
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
