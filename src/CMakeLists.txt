# Add executable
file(GLOB SOURCES *.cpp *.h)
add_executable(magnet ${SOURCES})

# Set include directories
target_include_directories(magnet PRIVATE ${PROJECT_SOURCE_DIR}/src)

# Link libraries
find_package(OpenGL REQUIRED)
target_link_libraries(magnet PRIVATE glad glfw glm::glm-header-only imgui imgui_toggle OpenGL::GL stb box2d sajson)

# Set executable directory
set_target_properties(magnet PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
# # Copy shaders directory to binary dir
# add_custom_command(
#     TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#             ${CMAKE_SOURCE_DIR}/src/shaders
#             ${CMAKE_BINARY_DIR}/bin/shaders
#     COMMENT "Copying shaders to binary directory")

# # Copy resources directory to binary dir
# add_custom_command(
#     TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#             ${CMAKE_SOURCE_DIR}/resources
#             ${CMAKE_BINARY_DIR}/bin/resources
#     COMMENT "Copying resources to binary directory")
