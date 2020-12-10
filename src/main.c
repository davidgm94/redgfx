#include "types.h"
#include "os.h"
#include "dependencies/volk.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <math.h>
#include <vulkan/vulkan_core.h>

#define VKCHECK(_result) do { VkResult result = _result; if (result == VK_SUCCESS) { print("%s OK\n", #_result); } else { print("%s FAIL! %s\n", #_result, VKResultToString(_result)); redassert(result == VK_SUCCESS); } } while(false);

static inline const char* VKResultToString(VkResult result)
{
    switch (result)
    {
        CASE_TO_STR(VK_SUCCESS);
        CASE_TO_STR(VK_NOT_READY);
        CASE_TO_STR(VK_TIMEOUT);
        CASE_TO_STR(VK_EVENT_SET);
        CASE_TO_STR(VK_EVENT_RESET);
        CASE_TO_STR(VK_INCOMPLETE);
        CASE_TO_STR(VK_ERROR_OUT_OF_HOST_MEMORY);
        CASE_TO_STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        CASE_TO_STR(VK_ERROR_INITIALIZATION_FAILED);
        CASE_TO_STR(VK_ERROR_DEVICE_LOST);
        CASE_TO_STR(VK_ERROR_MEMORY_MAP_FAILED);
        CASE_TO_STR(VK_ERROR_LAYER_NOT_PRESENT);
        CASE_TO_STR(VK_ERROR_EXTENSION_NOT_PRESENT);
        CASE_TO_STR(VK_ERROR_FEATURE_NOT_PRESENT);
        CASE_TO_STR(VK_ERROR_INCOMPATIBLE_DRIVER);
        CASE_TO_STR(VK_ERROR_TOO_MANY_OBJECTS);
        CASE_TO_STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
        CASE_TO_STR(VK_ERROR_FRAGMENTED_POOL);
        CASE_TO_STR(VK_ERROR_UNKNOWN);
        CASE_TO_STR(VK_ERROR_OUT_OF_POOL_MEMORY);
        CASE_TO_STR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
        CASE_TO_STR(VK_ERROR_FRAGMENTATION);
        CASE_TO_STR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
        CASE_TO_STR(VK_ERROR_SURFACE_LOST_KHR);
        CASE_TO_STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
        CASE_TO_STR(VK_SUBOPTIMAL_KHR);
        CASE_TO_STR(VK_ERROR_OUT_OF_DATE_KHR);
        CASE_TO_STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
        CASE_TO_STR(VK_ERROR_VALIDATION_FAILED_EXT);
        CASE_TO_STR(VK_ERROR_INVALID_SHADER_NV);
        CASE_TO_STR(VK_ERROR_INCOMPATIBLE_VERSION_KHR);
        CASE_TO_STR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
        CASE_TO_STR(VK_ERROR_NOT_PERMITTED_EXT);
        CASE_TO_STR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
        CASE_TO_STR(VK_THREAD_IDLE_KHR);
        CASE_TO_STR(VK_THREAD_DONE_KHR);
        CASE_TO_STR(VK_OPERATION_DEFERRED_KHR);
        CASE_TO_STR(VK_OPERATION_NOT_DEFERRED_KHR);
        CASE_TO_STR(VK_PIPELINE_COMPILE_REQUIRED_EXT);
        // CASE_TO_STR(VK_ERROR_OUT_OF_POOL_MEMORY_KHR);
        // CASE_TO_STR(VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR);
        // CASE_TO_STR(VK_ERROR_FRAGMENTATION_EXT);
        // CASE_TO_STR(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT);
        // CASE_TO_STR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR);
        // CASE_TO_STR(VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT);
        CASE_TO_STR(VK_RESULT_MAX_ENUM);
        default:
            RED_NOT_IMPLEMENTED;
            return null;
    }
}

static inline VkInstance create_instance(VkAllocationCallbacks* pAllocator, const char* engine_name, const char* application_name, u32 api_version, u32 engine_version, u32 application_version, const char* const* extensions, u32 extension_count, const char* const* layers, u32 layer_count)
{
    VkApplicationInfo application_info = ZERO_INIT;
    application_info.apiVersion = api_version;
    application_info.engineVersion = engine_version;
    application_info.applicationVersion = application_version;
    application_info.pApplicationName = application_name;
    application_info.pEngineName = engine_name;
    application_info.pNext = NULL;
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.applicationVersion = 1;

    VkInstanceCreateInfo create_info = ZERO_INIT;
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application_info;
    create_info.ppEnabledExtensionNames = extension_count > 0 ? extensions : NULL;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledLayerNames = layer_count > 0 ? layers : NULL;
    create_info.enabledLayerCount = layer_count;
    VkInstance instance;
    VKCHECK(vkCreateInstance(&create_info, pAllocator, &instance));

    return instance;
}

static inline void foo()
{
}

static VkBool32 VK_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        print("Error: ");
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        print("Warning: ");
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        print("Info: ");
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        print("Verbose: ");
    }
    print("%s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

static inline VkPhysicalDevice pick_physical_device(VkPhysicalDevice* device_buffer, u32 device_count)
{
    if (device_count > 0)
    {
        return device_buffer[0];
    }
    else
    {
        return NULL;
    }
}

static inline VkDebugUtilsMessengerEXT create_debug_utils_messenger(VkAllocationCallbacks* pAllocator, VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callback)
{
    VkDebugUtilsMessengerCreateInfoEXT create_info = ZERO_INIT;
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.pfnUserCallback = callback;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    VkDebugUtilsMessengerEXT messenger;
    VKCHECK(vkCreateDebugUtilsMessengerEXT(instance, &create_info, pAllocator, &messenger));

    return messenger;
}

static inline VkDevice create_device(VkAllocationCallbacks* pAllocator, VkPhysicalDevice pd, const char* const* device_extensions, u32 device_extension_count, u32* queue_family_indices, u32 queue_family_count, VkQueueFamilyProperties* queue_families)
{
    f32 queue_priorities[] = { 1.0f };
    VkDeviceQueueCreateInfo queue_create_infos[100] = ZERO_INIT;
    for (u32 i = 0; i < queue_family_count; i++)
    {
        u32 queue_family_index = queue_family_indices[i];
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = queue_family_index;
        queue_create_infos[i].queueCount = 1;
        queue_create_infos[i].pQueuePriorities = queue_priorities;
    }

    VkDeviceCreateInfo create_info = ZERO_INIT;
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.ppEnabledExtensionNames = device_extension_count > 0 ? device_extensions : NULL;
    create_info.enabledExtensionCount = device_extension_count;
    create_info.pQueueCreateInfos = queue_create_infos;
    create_info.queueCreateInfoCount = queue_family_count;

    VkDevice device;
    VKCHECK(vkCreateDevice(pd, &create_info, pAllocator, &device));
    return device;
}

static inline VkSurfaceFormatKHR get_swapchain_format(VkSurfaceFormatKHR* surface_formats, u32 surface_format_count)
{
    for (u32 i = 0; i < surface_format_count; i++)
    {
        if (surface_formats[i].format == VK_FORMAT_R8G8B8A8_UNORM || surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
        {
            return surface_formats[i];
        }
    }

    return surface_formats[0];
}

static inline VkSwapchainKHR create_swapchain(VkAllocationCallbacks* pAllocator, VkDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR surface_capabilities, VkSurfaceFormatKHR format, VkExtent2D extent, u32 queue_family_index)
{
    VkCompositeAlphaFlagBitsKHR surface_composite = (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
		? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
		: (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
		? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
		: (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
		? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
		: VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;


    VkSwapchainCreateInfoKHR create_info = ZERO_INIT;
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = MAX(2, surface_capabilities.minImageCount);
    create_info.imageFormat = format.format;
    create_info.imageColorSpace = format.colorSpace;
    create_info.imageExtent = extent;
    print("Image extent: %ux%u\n", create_info.imageExtent.width, create_info.imageExtent.height);
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    create_info.compositeAlpha = surface_composite;
    create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    create_info.clipped = VK_TRUE;
    create_info.pQueueFamilyIndices = &queue_family_index;
    create_info.queueFamilyIndexCount = 1;

    VkSwapchainKHR swapchain;
    VKCHECK(vkCreateSwapchainKHR(device, &create_info, pAllocator, &swapchain));

    return swapchain;
}

typedef struct Application
{
    char* title;
    s32 width, height;
    u32 version;
} Application;

s32 main(s32 argc, char* argv[])
{
    Application app =
    {
        .title = "Hello world",
        .width = 1024,
        .height = 768, 
        .version = 1,
    };
    
    s32 result = glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    volkInitialize();
    redassert(result == GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(app.width, app.height, app.title, NULL, NULL);
    redassert(window);

    VkAllocationCallbacks* pAllocator = NULL;

    // Layers
    VkLayerProperties instance_layers[200];
    const char* used_instance_layers[200];
    u32 instance_layer_count;
    u32 used_instance_layer_count = 0;
    VKCHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, null));
    VKCHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers));
    print("Instance layer count = %u\n", instance_layer_count);
    bool validation_layer_found = false;
    for (u32 i = 0; i < instance_layer_count; i++)
    {
        print("%s: %s\n", instance_layers[i].layerName, instance_layers[i].description);
        if (strcmp("VK_LAYER_KHRONOS_validation", instance_layers[i].layerName) == 0)
        {
            print("Found validation layer. Adding to the instance layers\n");
            used_instance_layers[instance_layer_count++] = instance_layers[i].layerName;
            validation_layer_found = true;
        }
    }

    // Extensions

    VkExtensionProperties instance_extensions[200] = ZERO_INIT;
    const char* used_instance_extensions[200] = ZERO_INIT;
    u32 instance_extension_count;
    u32 used_instance_extension_count;

    VKCHECK(vkEnumerateInstanceExtensionProperties(null, &instance_extension_count, null));
    VKCHECK(vkEnumerateInstanceExtensionProperties(null, &instance_extension_count, instance_extensions));
    print("Instance extension count = %u\n", instance_extension_count);
    const char* platform_surface_name = VK_KHR_XCB_SURFACE_EXTENSION_NAME;

    bool debug_utils_extension_found = false;
    bool surface_extension_found = false;
    bool platform_surface_extension_found = false;

    for (u32 i = 0; i < instance_extension_count; i++)
    {
        if (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, instance_extensions[i].extensionName) == 0)
        {
            print("Found debug utils extension! Adding it to the instance extensions\n");
            debug_utils_extension_found = true;
            used_instance_extensions[used_instance_extension_count++] = instance_extensions[i].extensionName;
            continue;
        }

        if (strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName) == 0)
        {
            print("Found surface extension! Adding it to the instance extensions\n");
            surface_extension_found = true;
            used_instance_extensions[used_instance_extension_count++] = instance_extensions[i].extensionName;
            continue;
        }

        if (strcmp(platform_surface_name, instance_extensions[i].extensionName) == 0)
        {
            print("Found platform surface extension! Adding it to the instance extensions\n");
            platform_surface_extension_found = true;
            used_instance_extensions[used_instance_extension_count++] = instance_extensions[i].extensionName;
            continue;
        }
    }

    redassert(debug_utils_extension_found && surface_extension_found && platform_surface_extension_found);

    VkInstance instance = create_instance(pAllocator, app.title, app.title, VK_API_VERSION_1_2, app.version, app.version, used_instance_extensions, used_instance_extension_count, used_instance_layers, used_instance_layer_count);
    volkLoadInstanceOnly(instance);

    create_debug_utils_messenger(pAllocator, instance, VK_debug_callback);

    VkPhysicalDevice physical_devices[16] = {0};
    u32 physical_device_count = array_length(physical_devices);
    VKCHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));
    VkPhysicalDevice pd = pick_physical_device(physical_devices, physical_device_count);

    // Device Layers are deprecated in Vulkan
    // VkLayerProperties device_layers[100];
    // u32 device_layer_count;
    // VKCHECK(vkEnumerateDeviceLayerProperties(pd, &device_layer_count, null));
    // VKCHECK(vkEnumerateDeviceLayerProperties(pd, &device_layer_count, device_layers));
    // print("Device layer count: %u\n", device_layer_count);
    // for (u32 i = 0; i < device_layer_count; i++)
    // {
    //     print("%s: %s\n", device_layers[i].layerName, device_layers[i].description);
    // }

    // Extensions
    VkExtensionProperties device_extensions[1024];
    u32 device_extension_count;
    const char* used_device_extensions[1024];
    u32 used_device_extension_count = 0;
    VKCHECK(vkEnumerateDeviceExtensionProperties(pd, null, &device_extension_count, null));
    VKCHECK(vkEnumerateDeviceExtensionProperties(pd, null, &device_extension_count, device_extensions));
    bool swapchain_extension_found = false;
    print("Device extension count: %u\n", device_extension_count);
    for (u32 i = 0; i < device_extension_count; i++)
    {
        print("%s\n", device_extensions[i].extensionName);
        if (strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName) == 0)
        {
            print("Found swapchain device extension\n");
            used_device_extensions[used_device_extension_count++] = device_extensions[i].extensionName;
            swapchain_extension_found = true;
            continue;
        }
    }

    redassert(swapchain_extension_found);
    // VkDisplayPropertiesKHR display_properties[100];
    // u32 display_property_count;
    // VKCHECK(vkGetPhysicalDeviceDisplayPropertiesKHR(pd, &display_property_count, null));
    // VKCHECK(vkGetPhysicalDeviceDisplayPropertiesKHR(pd, &display_property_count, display_properties));

    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(pd, &device_features);
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(pd, &device_properties);
    VkQueueFamilyProperties device_queue_families[100];
    u32 device_queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &device_queue_family_count, null);
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &device_queue_family_count, device_queue_families);
    u32 queue_family_index = UINT32_MAX;
    VkQueueFamilyProperties the_queue_family;
    for (u32 i = 0; i < device_queue_family_count; i++)
    {
        VkQueueFamilyProperties* queue_family = &device_queue_families[i];
        if (queue_family->queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queue_family_index = i;
            the_queue_family = *queue_family;
        }
    }

    redassert(queue_family_index != UINT32_MAX);

    VkSurfaceKHR surface;
    VKCHECK(glfwCreateWindowSurface(instance, window, pAllocator, &surface));

    VkDevice device = create_device(pAllocator, pd, used_device_extensions, used_device_extension_count, &queue_family_index, 1, &the_queue_family);
    volkLoadDevice(device);

    VkSurfaceFormatKHR surface_formats[100];
    u32 surface_format_count;
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &surface_format_count, null));
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &surface_format_count, surface_formats));
    VkSurfaceFormatKHR surface_format = get_swapchain_format(surface_formats, surface_format_count);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VKCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &surface_capabilities));
    VkExtent2D extent = surface_capabilities.currentExtent;

    VkSwapchainKHR swapchain = create_swapchain(pAllocator, device, surface, surface_capabilities, surface_format, extent, queue_family_index);

    VkImage swapchain_images[16];
    u32 swapchain_image_count;
    VKCHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, null));
    VKCHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images));

    VkImageView swapchain_image_views[16];

    for (u32 i = 0; i < swapchain_image_count; i++)
    {
        VkImageViewCreateInfo create_info = ZERO_INIT;
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swapchain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        create_info.components.a = VK_COMPONENT_SWIZZLE_A;
        create_info.format = surface_format.format;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.layerCount = 1;
        create_info.subresourceRange.levelCount = 1;
        
        VKCHECK(vkCreateImageView(device, &create_info, pAllocator, &swapchain_image_views[i]));
    }

    VkQueue queue;
    u32 queue_index = 0;
    vkGetDeviceQueue(device, queue_family_index, queue_index, &queue);
    redassert(queue);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo cb_create_info = ZERO_INIT;
    cb_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cb_create_info.queueFamilyIndex = queue_family_index;
    VKCHECK(vkCreateCommandPool(device, &cb_create_info, pAllocator, &command_pool));

    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo allocate_info = ZERO_INIT;
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = command_pool;
    allocate_info.commandBufferCount = 1;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VKCHECK(vkAllocateCommandBuffers(device, &allocate_info, &command_buffer));

    VkAttachmentDescription color_attachment = ZERO_INIT;
    color_attachment.format = surface_format.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = ZERO_INIT;
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc = ZERO_INIT;
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_attachment_ref;

    VkRenderPassCreateInfo rp_create_info = ZERO_INIT;
    rp_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_create_info.pAttachments = &color_attachment;
    rp_create_info.attachmentCount = 1;
    rp_create_info.pSubpasses = &subpass_desc;
    rp_create_info.subpassCount = 1;

    VkRenderPass render_pass;
    VKCHECK(vkCreateRenderPass(device, &rp_create_info, pAllocator, &render_pass));

    VkFramebufferCreateInfo fb_create_info =
    {
        0,
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .width = extent.width,
        .height = extent.height,
        .layers = 1,
    };

    VkFramebuffer framebuffers[16];

    for (u32 i = 0; i < swapchain_image_count; i++)
    {
        fb_create_info.pAttachments = &swapchain_image_views[i];
        VKCHECK(vkCreateFramebuffer(device, &fb_create_info, pAllocator, &framebuffers[i]));
    }

    VkFenceCreateInfo fence_create_info =
    {
        0,
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VkFence fence;
    VKCHECK(vkCreateFence(device, &fence_create_info, pAllocator, &fence));

    VkSemaphoreCreateInfo sem_create_info =
    {
        0,
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkSemaphore render_sem, present_sem;
    VKCHECK(vkCreateSemaphore(device, &sem_create_info, pAllocator, &render_sem));
    VKCHECK(vkCreateSemaphore(device, &sem_create_info, pAllocator, &present_sem));

    u32 frame_number = 0;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        VKCHECK(vkWaitForFences(device, 1, &fence, true, 1000 * 1000 * 1000));
        VKCHECK(vkResetFences(device, 1, &fence));

        u32 swapchain_image_index;
        VKCHECK(vkAcquireNextImageKHR(device, swapchain, 0, present_sem, null, &swapchain_image_index));

        VKCHECK(vkResetCommandBuffer(command_buffer, 0));

        VkCommandBufferBeginInfo begin_info =
        {
            0,
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        VKCHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

        VkClearValue clear_value = ZERO_INIT;
        f32 flash = (f32)fabs(sin(frame_number / 120.f));
        clear_value.color = (VkClearColorValue) { { 0.0f, 0.0f, flash, 1.0f} };

        VkRenderPassBeginInfo rp_begin_info =
        {
            0,
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass,
            .renderArea.extent = extent,
            .framebuffer = framebuffers[swapchain_image_index],
            .pClearValues = &clear_value,
            .clearValueCount = 1,
        };

        vkCmdBeginRenderPass(command_buffer, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdEndRenderPass(command_buffer);
        VKCHECK(vkEndCommandBuffer(command_buffer));

        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info =
        {
            0,
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pWaitDstStageMask = &wait_stage,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &present_sem,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_sem,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
        };

        VKCHECK(vkQueueSubmit(queue, 1, &submit_info, fence));

        VkPresentInfoKHR present_info =
        {
            0,
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pSwapchains = &swapchain,
            .swapchainCount = 1,
            .pWaitSemaphores = &render_sem,
            .waitSemaphoreCount = 1,
            .pImageIndices = &swapchain_image_index,
        };

        VKCHECK(vkQueuePresentKHR(queue, &present_info));

        frame_number++;

        //return 0;
    }
}
