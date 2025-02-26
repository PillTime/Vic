#ifndef VIC_GLFW_H
#define VIC_GLFW_H

#include <GLFW/glfw3.h>

void vicInitializeGlfw();
GLFWwindow *vicCreateWindow(size_t width, size_t height, char const *title,
                            GLFWframebuffersizefun resize_callback);
void vicDestroyWindow(GLFWwindow *window);
void vicTerminateGlfw();

#endif // VIC_GLFW_H
