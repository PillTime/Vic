add_subdirectory(utils)
add_subdirectory(glfw)
add_subdirectory(vulkan)

add_executable(vic main.c)
target_link_libraries(vic vicUtils vicGlfw vicVulkan)
