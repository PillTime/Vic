#include <stdlib.h>

#include <GLFW/glfw3.h>

GLFWwindow *g_window;

void vicInitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    g_window = glfwCreateWindow(1280, 720, "Vic", nullptr, nullptr);
}

void vicInitVulkan()
{
}

void vicMainLoop()
{
    while (!glfwWindowShouldClose(g_window))
    {
        glfwPollEvents();
    }
}

void vicCleanup()
{
    glfwDestroyWindow(g_window);
    glfwTerminate();
}

int main()
{
    vicInitWindow();
    vicInitVulkan();
    vicMainLoop();
    vicCleanup();

    return EXIT_SUCCESS;
}
