#include "types.h"
#include "os.h"
#include "dependencies/volk.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vulkan_xcb.h>

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

static inline VkInstance create_instance(VkAllocationCallbacks* pAllocator, const char* engine_name, const char* application_name, u32 api_version, u32 engine_version, u32 application_version, const char* const* extensions, u32 extension_count)
{
    VkApplicationInfo application_info = ZI0;
    application_info.apiVersion = api_version;
    application_info.engineVersion = engine_version;
    application_info.applicationVersion = application_version;
    application_info.pApplicationName = application_name;
    application_info.pEngineName = engine_name;
    application_info.pNext = NULL;
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.applicationVersion = 1;

    VkInstanceCreateInfo create_info = ZI0;
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &application_info;
    create_info.ppEnabledExtensionNames = extension_count > 0 ? extensions : NULL;
    create_info.enabledExtensionCount = extension_count;
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
    VkDebugUtilsMessengerCreateInfoEXT create_info = ZI0;
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.pfnUserCallback = callback;
    create_info.messageType = 0;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    VkDebugUtilsMessengerEXT messenger;
    VKCHECK(vkCreateDebugUtilsMessengerEXT(instance, &create_info, pAllocator, &messenger));

    return messenger;
}

static inline VkDevice create_device(VkAllocationCallbacks* pAllocator, VkPhysicalDevice pd)
{
    VkDeviceCreateInfo create_info = ZI0;
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    VkDevice device;
    VKCHECK(vkCreateDevice(pd, &create_info, pAllocator, &device));
    return device;
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

    VkExtensionProperties instance_extensions[200] = ZI0;
    const char* used_instance_extensions[200] = ZI0;
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

    VkInstance instance = create_instance(pAllocator, app.title, app.title, VK_API_VERSION_1_2, app.version, app.version, used_instance_extensions, used_instance_extension_count);
    volkLoadInstanceOnly(instance);

    create_debug_utils_messenger(pAllocator, instance, VK_debug_callback);

    VkPhysicalDevice physical_devices[16] = {0};
    u32 physical_device_count = array_length(physical_devices);
    VKCHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));
    VkPhysicalDevice pd = pick_physical_device(physical_devices, physical_device_count);

    VkSurfaceKHR surface;
    VKCHECK(glfwCreateWindowSurface(instance, window, pAllocator, &surface));

    VkDevice device = create_device(pAllocator, pd);
    volkLoadDevice(device);

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}
