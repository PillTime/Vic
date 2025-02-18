#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

uint32_t const k_WINDOW_WIDTH = 1280;
uint32_t const k_WINDOW_HEIGHT = 720;
char const *const k_APP_NAME = "vic";
char const *const k_DEVICE_EXTENSIONS[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
size_t const k_DEVICE_EXTENSIONS_COUNT = sizeof(k_DEVICE_EXTENSIONS) / sizeof(char const *);

#ifndef NDEBUG
char const *const k_VALIDATION_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};
size_t const k_VALIDATION_LAYERS_COUNT = sizeof(k_VALIDATION_LAYERS) / sizeof(char const *);
#endif

GLFWwindow *g_window;
VkInstance g_instance;
VkPhysicalDevice g_physical_device = VK_NULL_HANDLE;
VkDevice g_device;
VkSurfaceKHR g_surface;
VkQueue g_graphics_queue;
VkQueue g_present_queue;
VkSwapchainKHR g_swap_chain;
VkImage *g_swap_chain_images;
VkImageView *g_swap_chain_image_views;
uint32_t g_swap_chain_images_count;
VkFormat g_swap_chain_image_format;
VkExtent2D g_swap_chain_extent;
VkPipelineLayout g_pipeline_layout;

#ifndef NDEBUG
VkDebugUtilsMessengerEXT g_debug_messenger;
#endif

typedef struct S_QueueFamilyIndices
{
    bool has_graphics_family;
    uint32_t graphics_family;
    bool has_present_family;
    uint32_t present_family;
} QueueFamilyIndices;

QueueFamilyIndices vicNewQueueFamilyIndices()
{
    return (QueueFamilyIndices){.has_graphics_family = false, .has_present_family = false};
}

bool vicQueueFamilyIndicesIsComplete(QueueFamilyIndices const *const indices)
{
    return indices->has_graphics_family && indices->has_present_family;
}

typedef struct S_SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    uint32_t formats_count;
    VkPresentModeKHR *present_modes;
    uint32_t present_modes_count;
} SwapChainSupportDetails;

void vicCreateInstance();
char **vicGetRequiredExtensions_DM(uint32_t *const);
void vicCreateSurface();
void vicPickPhysicalDevice();
bool vicDeviceIsSuitable(VkPhysicalDevice const);
QueueFamilyIndices vicFindQueueFamilies(VkPhysicalDevice const);
void vicCreateLogicalDevice();
bool vicDeviceExtensionsSupported(VkPhysicalDevice const);
SwapChainSupportDetails vicQuerySwapChainSupport_DM(VkPhysicalDevice const);
VkSurfaceFormatKHR vicChooseSwapSurfaceFormat(VkSurfaceFormatKHR const *const, size_t const);
VkPresentModeKHR vicChooseSwapPresentMode(VkPresentModeKHR const *const, size_t const);
VkExtent2D vicChooseSwapExtent(VkSurfaceCapabilitiesKHR const *const);
void vicCreateSwapChain_DM();
void vicCreateImageViews_DM();
void vicCreateGraphicsPipeline();
char *vicReadFile_DM(char const *const, size_t *const);
VkShaderModule vicCreateShaderModule(char const *const, size_t const);

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

uint32_t vicClamp(uint32_t const min, uint32_t const x, uint32_t const max)
{
    return x < min ? min : x > max ? max : x;
}

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
    vicCreateSurface();
    vicPickPhysicalDevice();
    vicCreateLogicalDevice();
    vicCreateSwapChain_DM();
    vicCreateImageViews_DM();
    vicCreateGraphicsPipeline();
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
    vkDestroyPipelineLayout(g_device, g_pipeline_layout, nullptr);

    for (size_t i = 0; i < g_swap_chain_images_count; i++)
    {
        vkDestroyImageView(g_device, g_swap_chain_image_views[i], nullptr);
    }
    free(g_swap_chain_image_views);

    free(g_swap_chain_images);
    vkDestroySwapchainKHR(g_device, g_swap_chain, nullptr);

    vkDestroyDevice(g_device, nullptr);

#ifndef NDEBUG
    vicDestroyDebugUtilsMessengerEXT(g_instance, g_debug_messenger, nullptr);
#endif

    vkDestroySurfaceKHR(g_instance, g_surface, nullptr);
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

void vicCreateSurface()
{
    if (glfwCreateWindowSurface(g_instance, g_window, nullptr, &g_surface) != VK_SUCCESS)
    {
        vicDie("failed to create window surface");
    }
}

void vicPickPhysicalDevice()
{
    uint32_t devices_count = 0;
    vkEnumeratePhysicalDevices(g_instance, &devices_count, nullptr);
    if (devices_count == 0)
    {
        vicDie("failed to find GPU with vulkan support");
    }

    VkPhysicalDevice *const devices = calloc((size_t)devices_count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(g_instance, &devices_count, devices);

    for (size_t i = 0; i < devices_count; i++)
    {
        if (vicDeviceIsSuitable(devices[i]))
        {
            g_physical_device = devices[i];
            break;
        }
    }
    if (g_physical_device == VK_NULL_HANDLE)
    {
        vicDie("failed to find suitable GPU");
    }

    free(devices);
}

bool vicDeviceIsSuitable(VkPhysicalDevice const device)
{
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(device, &properties);
    VkPhysicalDeviceFeatures features = {};
    vkGetPhysicalDeviceFeatures(device, &features);

    QueueFamilyIndices const indices = vicFindQueueFamilies(device);

    bool const extensions_supported = vicDeviceExtensionsSupported(device);

    bool swap_chain_adequate = false;
    if (extensions_supported)
    {
        SwapChainSupportDetails const swap_chain_support = vicQuerySwapChainSupport_DM(device);
        swap_chain_adequate =
            swap_chain_support.formats_count != 0 && swap_chain_support.present_modes_count != 0;

        free(swap_chain_support.formats);
        free(swap_chain_support.present_modes);
    }

    return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           features.geometryShader && vicQueueFamilyIndicesIsComplete(&indices) &&
           extensions_supported && swap_chain_adequate;
}

QueueFamilyIndices vicFindQueueFamilies(VkPhysicalDevice const device)
{
    QueueFamilyIndices indices = vicNewQueueFamilyIndices();

    uint32_t queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, nullptr);
    VkQueueFamilyProperties *const queue_families =
        calloc((size_t)queue_families_count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_families_count, queue_families);

    // TODO: check why validation layers complain about non unique queue family indices
    for (size_t i = 0; i < queue_families_count; i++)
    {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphics_family = i;
            indices.has_graphics_family = true;
            continue;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, g_surface, &present_support);
        if (present_support)
        {
            indices.present_family = i;
            indices.has_present_family = true;
            continue;
        }

        if (vicQueueFamilyIndicesIsComplete(&indices))
        {
            break;
        }
    }

    free(queue_families);
    return indices;
}

void vicCreateLogicalDevice()
{
    QueueFamilyIndices const indices = vicFindQueueFamilies(g_physical_device);

    uint32_t const unique_queue_families[] = {indices.graphics_family, indices.present_family};
    size_t const unique_queue_families_count = sizeof(unique_queue_families) / sizeof(uint32_t);

    VkDeviceQueueCreateInfo *const queue_create_infos =
        calloc(unique_queue_families_count, sizeof(VkDeviceQueueCreateInfo));

    float const queue_priority = 1.0;
    for (size_t i = 0; i < unique_queue_families_count; i++)
    {
        queue_create_infos[i] = (VkDeviceQueueCreateInfo){};
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = unique_queue_families[i];
        queue_create_infos[i].queueCount = 1;
        queue_create_infos[i].pQueuePriorities = &queue_priority;
    }

    VkPhysicalDeviceFeatures const device_features = {};

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_infos;
    create_info.queueCreateInfoCount = (uint32_t)unique_queue_families_count;
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = (uint32_t)k_DEVICE_EXTENSIONS_COUNT;
    create_info.ppEnabledExtensionNames = k_DEVICE_EXTENSIONS;
#ifndef NDEBUG
    create_info.enabledLayerCount = (uint32_t)k_VALIDATION_LAYERS_COUNT;
    create_info.ppEnabledLayerNames = k_VALIDATION_LAYERS;
#else
    create_info.enabledLayerCount = 0;
#endif

    if (vkCreateDevice(g_physical_device, &create_info, nullptr, &g_device) != VK_SUCCESS)
    {
        vicDie("failed to create logical device");
    }

    vkGetDeviceQueue(g_device, indices.graphics_family, 0, &g_graphics_queue);
    vkGetDeviceQueue(g_device, indices.present_family, 0, &g_present_queue);

    free(queue_create_infos);
}

bool vicDeviceExtensionsSupported(VkPhysicalDevice const device)
{
    uint32_t extensions_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, nullptr);
    VkExtensionProperties *const extensions =
        calloc(extensions_count, sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensions_count, extensions);

    bool all_extensions_available = true;

    for (size_t i = 0; i < k_DEVICE_EXTENSIONS_COUNT; i++)
    {
        bool extension_found = false;
        for (size_t j = 0; j < extensions_count; j++)
        {
            if (strcmp(k_DEVICE_EXTENSIONS[i], extensions[j].extensionName) == 0)
            {
                extension_found = true;
                break;
            }
        }
        all_extensions_available &= extension_found;
    }

    free(extensions);
    return all_extensions_available;
}

SwapChainSupportDetails vicQuerySwapChainSupport_DM(VkPhysicalDevice const device)
{
    SwapChainSupportDetails details = {};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, g_surface, &details.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_surface, &details.formats_count, nullptr);
    if (details.formats_count != 0)
    {
        details.formats = calloc(details.formats_count, sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, g_surface, &details.formats_count,
                                             details.formats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_surface, &details.present_modes_count,
                                              nullptr);
    if (details.present_modes_count != 0)
    {
        details.present_modes = calloc(details.present_modes_count, sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, g_surface, &details.present_modes_count,
                                                  details.present_modes);
    }

    return details;
}

VkSurfaceFormatKHR vicChooseSwapSurfaceFormat(VkSurfaceFormatKHR const *const available_formats,
                                              size_t const available_formats_count)
{
    for (size_t i = 0; i < available_formats_count; i++)
    {
        if (available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return available_formats[i];
        }
    }

    return available_formats[0];
}

VkPresentModeKHR vicChooseSwapPresentMode(VkPresentModeKHR const *const available_present_modes,
                                          size_t const available_present_modes_count)
{
    for (size_t i = 0; i < available_present_modes_count; i++)
    {
        if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return available_present_modes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D vicChooseSwapExtent(VkSurfaceCapabilitiesKHR const *const capabilities)
{
    if (capabilities->currentExtent.width != UINT32_MAX)
    {
        return capabilities->currentExtent;
    }
    else
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(g_window, &width, &height);

        VkExtent2D actual_extent = {
            .width = (uint32_t)width,
            .height = (uint32_t)height,
        };

        actual_extent.width = vicClamp(capabilities->minImageExtent.width, actual_extent.width,
                                       capabilities->maxImageExtent.width);
        actual_extent.height = vicClamp(capabilities->minImageExtent.height, actual_extent.height,
                                        capabilities->maxImageExtent.height);

        return actual_extent;
    }
}

void vicCreateSwapChain_DM()
{
    SwapChainSupportDetails const swap_chain_support =
        vicQuerySwapChainSupport_DM(g_physical_device);

    VkSurfaceFormatKHR const surface_format =
        vicChooseSwapSurfaceFormat(swap_chain_support.formats, swap_chain_support.formats_count);
    VkPresentModeKHR const present_mode = vicChooseSwapPresentMode(
        swap_chain_support.present_modes, swap_chain_support.present_modes_count);
    VkExtent2D const extent = vicChooseSwapExtent(&swap_chain_support.capabilities);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount)
    {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = g_surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices const indices = vicFindQueueFamilies(g_physical_device);
    uint32_t const queue_family_indices[] = {indices.graphics_family, indices.present_family};

    if (indices.graphics_family != indices.present_family)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        // TODO: same as in vicFindQueueFamilyIndices (this block will never run)
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(g_device, &create_info, nullptr, &g_swap_chain) != VK_SUCCESS)
    {
        vicDie("failed to create swap chain");
    }

    vkGetSwapchainImagesKHR(g_device, g_swap_chain, &g_swap_chain_images_count, nullptr);
    g_swap_chain_images = calloc(g_swap_chain_images_count, sizeof(VkImage));
    vkGetSwapchainImagesKHR(g_device, g_swap_chain, &g_swap_chain_images_count,
                            g_swap_chain_images);

    g_swap_chain_image_format = surface_format.format;
    g_swap_chain_extent = extent;

    free(swap_chain_support.formats);
    free(swap_chain_support.present_modes);
}

void vicCreateImageViews_DM()
{
    g_swap_chain_image_views = calloc(g_swap_chain_images_count, sizeof(VkImageView));
    for (size_t i = 0; i < g_swap_chain_images_count; i++)
    {
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = g_swap_chain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = g_swap_chain_image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(g_device, &create_info, nullptr, &g_swap_chain_image_views[i]) !=
            VK_SUCCESS)
        {
            vicDie("failed to create image views");
        }
    }
}

void vicCreateGraphicsPipeline()
{
    size_t vert_shader_code_size = 0;
    char *const vert_shader_code =
        vicReadFile_DM("build/shd/shader.vert.spv", &vert_shader_code_size);
    size_t frag_shader_code_size = 0;
    char *const frag_shader_code =
        vicReadFile_DM("build/shd/shader.frag.spv", &frag_shader_code_size);

    VkShaderModule const vert_shader_module =
        vicCreateShaderModule(vert_shader_code, vert_shader_code_size);
    VkShaderModule const frag_shader_module =
        vicCreateShaderModule(frag_shader_code, frag_shader_code_size);

    VkPipelineShaderStageCreateInfo vert_shader_stage_create_info = {};
    vert_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_create_info.module = vert_shader_module;
    vert_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_create_info = {};
    frag_shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_create_info.module = frag_shader_module;
    frag_shader_stage_create_info.pName = "main";

    VkPipelineShaderStageCreateInfo const shader_stages[] = {vert_shader_stage_create_info,
                                                             frag_shader_stage_create_info};
    (void)shader_stages;

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.vertexBindingDescriptionCount = 0;
    vertex_input_create_info.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {};
    input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0;
    viewport.y = 0.0;
    viewport.width = (float)g_swap_chain_extent.width;
    viewport.height = (float)g_swap_chain_extent.height;
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    VkRect2D scissor = {};
    scissor.offset = (VkOffset2D){.x = 0, .y = 0};
    scissor.extent = g_swap_chain_extent;

    VkDynamicState const dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
    dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_create_info.dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState);
    dynamic_create_info.pDynamicStates = dynamic_states;

    VkPipelineViewportStateCreateInfo viewport_create_info = {};
    viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_create_info.viewportCount = 1;
    viewport_create_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
    rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_create_info.depthClampEnable = VK_FALSE;
    rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_create_info.lineWidth = 1.0;
    rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_create_info.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
    multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_create_info.sampleShadingEnable = VK_FALSE;
    multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_create_info.minSampleShading = 1.0;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
    color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_create_info.logicOpEnable = VK_FALSE;
    color_blend_create_info.attachmentCount = 1;
    color_blend_create_info.pAttachments = &color_blend_attachment;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(g_device, &pipeline_layout_create_info, nullptr,
                               &g_pipeline_layout) != VK_SUCCESS)
    {
        vicDie("failed to create pipeline layout");
    }

    vkDestroyShaderModule(g_device, frag_shader_module, nullptr);
    vkDestroyShaderModule(g_device, vert_shader_module, nullptr);

    free(vert_shader_code);
    free(frag_shader_code);
}

char *vicReadFile_DM(char const *const filename, size_t *const file_size)
{
    FILE *const file = fopen(filename, "rb");
    if (!file)
    {
        vicDie("failed to open file");
    }

    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    rewind(file);

    char *const buffer = calloc(*file_size, sizeof(char));
    fread(buffer, 1, *file_size, file);

    fclose(file);
    return buffer;
}

VkShaderModule vicCreateShaderModule(char const *const code, size_t const code_size)
{
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code_size;
    create_info.pCode = (uint32_t const *)code;

    VkShaderModule shader_module;
    if (vkCreateShaderModule(g_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        vicDie("failed to create shader module");
    }
    return shader_module;
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
