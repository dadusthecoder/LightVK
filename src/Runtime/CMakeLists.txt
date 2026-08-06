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

# Copy Assets to output directory
if(EXISTS "${CMAKE_SOURCE_DIR}/Assets")
    add_custom_command(TARGET LightVK_Runtime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Assets"
            "$<TARGET_FILE_DIR:LightVK_Runtime>/Assets"
        COMMENT "Copying Assets → runtime output"
    )
endif()
