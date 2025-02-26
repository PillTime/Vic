#include "glfw.h"

#include <GLFW/glfw3.h>

void vicInitializeGlfw()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

GLFWwindow *vicCreateWindow(size_t const width, size_t const height, char const *const title,
                            GLFWframebuffersizefun const resize_callback)
{
    GLFWwindow *const g_window = glfwCreateWindow((int)width, (int)height, title, nullptr, nullptr);

    if (resize_callback != nullptr)
    {
        glfwSetFramebufferSizeCallback(g_window, resize_callback);
    }

    return g_window;
}

void vicDestroyWindow(GLFWwindow *const window)
{
    glfwDestroyWindow(window);
}

void vicTerminateGlfw()
{
    glfwTerminate();
}
