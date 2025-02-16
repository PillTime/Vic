#include <stdio.h>
#include <stdlib.h>

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE

#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <vulkan/vulkan.h>

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *const window = glfwCreateWindow(1280, 720, "Vic", nullptr, nullptr);

    uint32_t extensions_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);
    printf("%d extensions supported\n", extensions_count);

    mat4 matrix;
    glm_mat4_zero(matrix);
    vec4 vector;
    glm_vec4_zero(vector);
    vec4 result;
    glm_mat4_mulv(matrix, vector, result);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
