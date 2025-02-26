#include <inttypes.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>

#include "glfw/glfw.h"
#include "utils/utils.h"
#include "vulkan/vulkan.h"

void vicProcessArguments(int const argc, char const *const *const argv, size_t *const width,
                         size_t *const height)
{
    if (argc != 1 && argc != 3)
    {
        vicDie("expected no arguments, or two arguments (width and height)");
    }

    if (argc != 3)
    {
        return;
    }

    *width = (size_t)strtoumax(argv[1], nullptr, 10);
    *height = (size_t)strtoumax(argv[2], nullptr, 10);

    if (*width == 0 || *height == 0)
    {
        vicDie("invalid width and/or height values provided");
    }
}

void vicLoop(GLFWwindow *const window)
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}

int main(int const argc, char const *const *const argv)
{
    // default window resolution
    size_t width = 640, height = 480;
    vicProcessArguments(argc, argv, &width, &height);

    vicInitializeGlfw();
    GLFWwindow *const window = vicCreateWindow(width, height, "vic", nullptr);

    vicInitializeVulkan();
    vicCreateDevices();

    // loop-de-loop
    vicLoop(window);

    vicDestroyDevices();
    vicTerminateVulkan();

    vicDestroyWindow(window);
    vicTerminateGlfw();

    return EXIT_SUCCESS;
}
