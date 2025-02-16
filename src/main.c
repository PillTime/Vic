#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

uint32_t const k_WINDOW_WIDTH = 1280;
uint32_t const k_WINDOW_HEIGHT = 720;
char const *const k_APP_NAME = "vic";

#ifndef NDEBUG
char const *const k_VALIDATION_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};
size_t const k_VALIDATION_LAYERS_COUNT = sizeof(k_VALIDATION_LAYERS) / sizeof(char const *);
#endif

GLFWwindow *g_window;
VkInstance g_instance;

#ifndef NDEBUG
VkDebugUtilsMessengerEXT g_debug_messenger;
#endif

void vicCreateInstance();
char **vicGetRequiredExtensions_DM(uint32_t *const);

#ifndef NDEBUG
void vicPopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT *const);
void vicSetupDebugMessenger();
bool vicValidationLayersSupported();

VkResult vicCreateDebugUtilsMessengerEXT(VkInstance const,
                                         VkDebugUtilsMessengerCreateInfoEXT const *const,
                                         VkAllocationCallbacks const *const,
                                         VkDebugUtilsMessengerEXT *const);
void vicDestroyDebugUtilsMessengerEXT(VkInstance const, VkDebugUtilsMessengerEXT const,
                                      VkAllocationCallbacks const *const);
VKAPI_ATTR VkBool32 VKAPI_CALL vicDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT const,
                                                VkDebugUtilsMessageTypeFlagsEXT const,
                                                VkDebugUtilsMessengerCallbackDataEXT const *const,
                                                void *const);
#endif

void vicDie(char const *const msg)
{
    fputs(msg, stderr);
    fflush(stderr);
    abort();
}

void vicInitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    g_window = glfwCreateWindow(k_WINDOW_WIDTH, k_WINDOW_HEIGHT, k_APP_NAME, nullptr, nullptr);
}

void vicInitVulkan()
{
    vicCreateInstance();
#ifndef NDEBUG
    vicSetupDebugMessenger();
#endif
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
#ifndef NDEBUG
    vicDestroyDebugUtilsMessengerEXT(g_instance, g_debug_messenger, nullptr);
#endif

    vkDestroyInstance(g_instance, nullptr);

    glfwDestroyWindow(g_window);
    glfwTerminate();
}

void vicCreateInstance()
{
#ifndef NDEBUG
    if (!vicValidationLayersSupported())
    {
        vicDie("validation layers not supported");
    }
#endif

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Vic";
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 1);
    app_info.apiVersion = VK_API_VERSION_1_4;

    uint32_t required_extensions_count = 0;
    char **const required_extensions = vicGetRequiredExtensions_DM(&required_extensions_count);

#ifndef NDEBUG
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
    vicPopulateDebugMessengerCreateInfo(&debug_create_info);
#endif

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = required_extensions_count;
    create_info.ppEnabledExtensionNames = (char const *const *)required_extensions;
#ifndef NDEBUG
    create_info.pNext = &debug_create_info;
    create_info.enabledLayerCount = (uint32_t)k_VALIDATION_LAYERS_COUNT;
    create_info.ppEnabledLayerNames = k_VALIDATION_LAYERS;
#else
    create_info.enabledLayerCount = 0;
#endif

    uint32_t extensions_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);
    VkExtensionProperties *const extensions =
        calloc((size_t)extensions_count, sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, extensions);

    printf("%" PRIu32 " extensions available:", extensions_count);
    for (size_t i = 0; i < extensions_count; i++)
    {
        printf("\n\t%s", extensions[i].extensionName);
    }
    printf("\n");
    free(extensions);

    if (vkCreateInstance(&create_info, nullptr, &g_instance) != VK_SUCCESS)
    {
        vicDie("failed to create vulkan instance");
    }

    for (size_t i = 0; i < required_extensions_count; i++)
    {
        free(required_extensions[i]);
    }
    free(required_extensions);
}

char **vicGetRequiredExtensions_DM(uint32_t *const size)
{
    uint32_t glfw_extensions_count = 0;
    char const *const *const glfw_extensions =
        glfwGetRequiredInstanceExtensions(&glfw_extensions_count);

    *size = glfw_extensions_count;
#ifndef NDEBUG
    *size += 1;
#endif
    char **const extensions = calloc((size_t)*size, sizeof(char const *));

    for (size_t i = 0; i < glfw_extensions_count; i++)
    {
        size_t name_size = strlen(glfw_extensions[i]) + 1;
        extensions[i] = calloc(name_size, sizeof(char));
        memcpy(extensions[i], glfw_extensions[i], name_size / sizeof(char));
    }

#ifndef NDEBUG
    size_t extension_name_size = strlen(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) + 1;
    extensions[glfw_extensions_count] = calloc(extension_name_size, sizeof(char));
    memcpy(extensions[glfw_extensions_count], VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
           extension_name_size / sizeof(char));
#endif

    return extensions;
}

#ifndef NDEBUG
void vicPopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT *const create_info)
{
    create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    create_info->pfnUserCallback = vicDebugCallback;
}

void vicSetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT create_info = {};
    vicPopulateDebugMessengerCreateInfo(&create_info);

    if (vicCreateDebugUtilsMessengerEXT(g_instance, &create_info, nullptr, &g_debug_messenger) !=
        VK_SUCCESS)
    {
        vicDie("failed to setup debug messenger");
    }
}

bool vicValidationLayersSupported()
{
    uint32_t layers_count = 0;
    vkEnumerateInstanceLayerProperties(&layers_count, nullptr);
    VkLayerProperties *const layers = calloc((size_t)layers_count, sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layers_count, layers);

    for (size_t i = 0; i < k_VALIDATION_LAYERS_COUNT; i++)
    {
        bool layer_found = false;

        for (size_t j = 0; j < layers_count; j++)
        {
            if (strcmp(k_VALIDATION_LAYERS[i], layers[j].layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
        {
            free(layers);
            return false;
        }
    }

    free(layers);
    return true;
}

VkResult vicCreateDebugUtilsMessengerEXT(
    VkInstance const instance, VkDebugUtilsMessengerCreateInfoEXT const *const create_info,
    VkAllocationCallbacks const *const allocator, VkDebugUtilsMessengerEXT *const debug_messenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, create_info, allocator, debug_messenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void vicDestroyDebugUtilsMessengerEXT(VkInstance const instance,
                                      VkDebugUtilsMessengerEXT const debug_messenger,
                                      VkAllocationCallbacks const *const allocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debug_messenger, allocator);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL
vicDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT const severity,
                 VkDebugUtilsMessageTypeFlagsEXT const type,
                 VkDebugUtilsMessengerCallbackDataEXT const *const callback_data,
                 [[maybe_unused]] void *const user_data)
{
    FILE *output;
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        output = stderr;
    }
    else
    {
        output = stdout;
    }

    char const *prefix;
    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        prefix = "[GEN]";
    }
    else if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        prefix = "[PER]";
    }
    else if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        prefix = "[VAL]";
    }
    else
    {
        prefix = "[???]";
    }

    fprintf(output, "%s validation layer: %s\n", prefix, callback_data->pMessage);
    return VK_FALSE;
}
#endif

int main()
{
    vicInitWindow();
    vicInitVulkan();
    vicMainLoop();
    vicCleanup();

    return EXIT_SUCCESS;
}
