#include "types.h"
#include "os.h"
#include "dependencies/volk.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <math.h>
#include <vulkan/vulkan_core.h>
#ifdef RED_OS_WINDOWS
#include <spirv-headers/spirv.h>
#else
#include <spirv/1.2/spirv.h>
#endif

typedef struct Application
{
    char* title;
    s32 width, height;
    u32 version;
} Application;

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
        CASE_TO_STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR); CASE_TO_STR(VK_SUBOPTIMAL_KHR); CASE_TO_STR(VK_ERROR_OUT_OF_DATE_KHR);
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

static inline const char* present_mode_string(VkPresentModeKHR present_mode)
{
    switch(present_mode)
    {
        CASE_TO_STR(VK_PRESENT_MODE_IMMEDIATE_KHR);
        CASE_TO_STR(VK_PRESENT_MODE_MAILBOX_KHR);
        CASE_TO_STR(VK_PRESENT_MODE_FIFO_KHR);
        CASE_TO_STR(VK_PRESENT_MODE_FIFO_RELAXED_KHR);
        CASE_TO_STR(VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR);
        CASE_TO_STR(VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR);
        CASE_TO_STR(VK_PRESENT_MODE_MAX_ENUM_KHR);
        default:
            RED_NOT_IMPLEMENTED;
            return null;
    }
}

static inline VkInstance create_instance(VkAllocationCallbacks* pAllocator, const char* engine_name, const char* application_name, u32 api_version, u32 engine_version, u32 application_version, const char* const* extensions, u32 extension_count, const char* const* layers, u32 layer_count, VkDebugUtilsMessengerCreateInfoEXT* debug_utils)
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

    VkInstanceCreateInfo instance_ci = ZERO_INIT;
    instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_ci.pNext = debug_utils;
    instance_ci.pApplicationInfo = &application_info;
    instance_ci.ppEnabledExtensionNames = extension_count > 0 ? extensions : NULL;
    instance_ci.enabledExtensionCount = extension_count;
    instance_ci.ppEnabledLayerNames = layer_count > 0 ? layers : NULL;
    instance_ci.enabledLayerCount = layer_count;

    VkInstance instance;
    VKCHECK(vkCreateInstance(&instance_ci, pAllocator, &instance));

    return instance;
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

static inline VkDevice create_device(VkAllocationCallbacks* pAllocator, VkPhysicalDevice pd, const char* const* device_extensions, u32 device_extension_count, u32* queue_family_indices, u32 queue_family_count)
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

    VkDeviceCreateInfo device_ci = ZERO_INIT;
    device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_ci.ppEnabledExtensionNames = device_extension_count > 0 ? device_extensions : NULL;
    device_ci.enabledExtensionCount = device_extension_count;
    device_ci.pQueueCreateInfos = queue_create_infos;
    device_ci.queueCreateInfoCount = queue_family_count;
    //device_ci.enabledLayerCount = 1;
    //char validation_layer[] = "VK_LAYER_KHRONOS_validation";
    //device_ci.ppEnabledLayerNames = (const char* const*)&validation_layer;

    VkDevice device;
    VKCHECK(vkCreateDevice(pd, &device_ci, pAllocator, &device));
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


    VkSwapchainCreateInfoKHR swapchain_ci = ZERO_INIT;
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.surface = surface;
    swapchain_ci.minImageCount = MAX(2, surface_capabilities.minImageCount);
    swapchain_ci.imageFormat = format.format;
    swapchain_ci.imageColorSpace = format.colorSpace;
    swapchain_ci.imageExtent = extent;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_ci.compositeAlpha = surface_composite;
    swapchain_ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_ci.clipped = VK_TRUE;
    swapchain_ci.pQueueFamilyIndices = &queue_family_index;
    swapchain_ci.queueFamilyIndexCount = 1;

    VkSwapchainKHR swapchain;
    VKCHECK(vkCreateSwapchainKHR(device, &swapchain_ci, pAllocator, &swapchain));

    return swapchain;
}

static inline VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits shader_flags, VkShaderModule shader_module)
{
    VkPipelineShaderStageCreateInfo shader_stage_ci = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = shader_flags,
        .module = shader_module,
        .pName = "main",
    };

    return shader_stage_ci;
}

static inline VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_create_info(VkPrimitiveTopology topology)
{
    VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = topology,
    };

    return input_assembly_ci;
}

static inline VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygon_mode)
{

    VkPipelineRasterizationStateCreateInfo rasterization_state_ci =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = polygon_mode,
        .lineWidth = 1.0f,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .cullMode = VK_CULL_MODE_NONE,
    };

    return rasterization_state_ci;
}

typedef struct SpirVToken
{
    u32 op_code;
    u32 type_id;
    u32 storage_class;
    u32 binding;
    u32 set;
} SpirVToken;

static inline VkShaderStageFlagBits get_shader_stage(SpvExecutionModel execution_model)
{
    switch (execution_model)
    {
        case SpvExecutionModelVertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case SpvExecutionModelFragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case SpvExecutionModelGLCompute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            RED_UNREACHABLE;
            return 0;
    }
}

typedef struct Shader
{
    VkDescriptorType resource_types[32];
    u32 resource_mask;
    u32 local_size_x;
    u32 local_size_y;
    u32 local_size_z;
    bool uses_push_constants;
} Shader;

static inline Shader parse_shader(const u32* code, u32 codeSize, VkShaderStageFlagBits* shader_stage)
{
    Shader shader = ZERO_INIT;
	redassert(code[0] == SpvMagicNumber);

	u32 token_count = code[3];
    print("Token count = %u\n", token_count);

    SpirVToken tokens[1000];
	const u32* insn = code + 5;

    u32 first_iteration_count = 0;
	while (insn != code + codeSize)
	{
        print("Condition iterator: %u. Target: %u\n", insn, code + codeSize);
		u16 op_code = (u16)(insn[0]);
		u16 wordCount = (u16)(insn[0] >> 16);

		switch (op_code)
		{
		case SpvOpEntryPoint:
		{
			redassert(wordCount >= 2);
			*shader_stage = get_shader_stage((SpvExecutionModel)(insn[1]));
            return shader;
		} break;
		case SpvOpExecutionMode:
		{
			redassert(wordCount >= 3);
			u32 mode = insn[2];

			switch (mode)
			{
			case SpvExecutionModeLocalSize:
				redassert(wordCount == 6);
				shader.local_size_x = insn[3];
				shader.local_size_y = insn[4];
				shader.local_size_z = insn[5];
				break;
			}
		} break;
		case SpvOpDecorate:
		{
			redassert(wordCount >= 3);

			u32 id = insn[1];
			redassert(id < token_count);

			switch (insn[2])
			{
			case SpvDecorationDescriptorSet:
				redassert(wordCount == 4);
				tokens[id].set = insn[3];
				break;
			case SpvDecorationBinding:
				redassert(wordCount == 4);
				tokens[id].binding = insn[3];
				break;
			}
		} break;
		case SpvOpTypeStruct:
		case SpvOpTypeImage:
		case SpvOpTypeSampler:
		case SpvOpTypeSampledImage:
		{
			redassert(wordCount >= 2);

			u32 id = insn[1];
			redassert(id < token_count);

			redassert(tokens[id].op_code == 0);
			tokens[id].op_code = op_code;
		} break;
		case SpvOpTypePointer:
		{
			redassert(wordCount == 4);

			u32 id = insn[1];
			redassert(id < token_count);

			redassert(tokens[id].op_code == 0);
			tokens[id].op_code = op_code;
			tokens[id].type_id = insn[3];
			tokens[id].storage_class = insn[2];
		} break;
		case SpvOpVariable:
		{
			redassert(wordCount >= 4);

			u32 id = insn[2];
			redassert(id < token_count);

			redassert(tokens[id].op_code == 0);
			tokens[id].op_code = op_code;
			tokens[id].type_id = insn[1];
			tokens[id].storage_class = insn[3];
		} break;
		}

		redassert(insn + wordCount <= code + codeSize);
		insn += wordCount;
        //print("First loop iteration count = %u\n", ++first_iteration_count);
	}

    for (u32 i = 0; i < token_count; i++)
    {
        SpirVToken token = tokens[i];
		if (token.op_code == SpvOpVariable && (token.storage_class == SpvStorageClassUniform || token.storage_class == SpvStorageClassUniformConstant || token.storage_class == SpvStorageClassStorageBuffer))
		{
			redassert(token.set == 0);
			redassert(token.binding < 32);
			redassert(tokens[token.type_id].op_code == SpvOpTypePointer);

			redassert((shader.resource_mask & (1 << token.binding)) == 0);

			u32 typeKind = tokens[tokens[token.type_id].type_id].op_code;

			switch (typeKind)
			{
			case SpvOpTypeStruct:
				shader.resource_types[token.binding] = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				shader.resource_mask |= 1 << token.binding;
				break;
			case SpvOpTypeImage:
				shader.resource_types[token.binding] = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				shader.resource_mask |= 1 << token.binding;
				break;
			case SpvOpTypeSampler:
				shader.resource_types[token.binding] = VK_DESCRIPTOR_TYPE_SAMPLER;
				shader.resource_mask |= 1 << token.binding;
				break;
			case SpvOpTypeSampledImage:
				shader.resource_types[token.binding] = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				shader.resource_mask |= 1 << token.binding;
				break;
			default:
				redassert(!"Unknown resource type");
			}
		}

		if (token.op_code == SpvOpVariable && token.storage_class == SpvStorageClassPushConstant)
		{
			shader.uses_push_constants = true;
		}
	}

    print("Ended parsing a shader\n");

    return shader;
}

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
        // if (strcmp("VK_LAYER_LUNARG_standard_validation", instance_layers[i].layerName) == 0)
        // {
        //     print("Found validation layer. Adding to the instance layers\n");
        //     used_instance_layers[instance_layer_count++] = instance_layers[i].layerName;
        //     validation_layer_found = true;
        // }
    }

    // Extensions

    VkExtensionProperties instance_extensions[200] = ZERO_INIT;
    const char* used_instance_extensions[200] = ZERO_INIT;
    u32 instance_extension_count;
    u32 used_instance_extension_count;

    u32 glfw_extension_count;
    const char** extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    if (extensions)
    {
        for (u32 i = 0; i < glfw_extension_count; i++)
        {
            used_instance_extensions[used_instance_extension_count++] = extensions[i];
        }
    }

    used_instance_extensions[used_instance_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    VkDebugUtilsMessengerCreateInfoEXT debug_ci = ZERO_INIT;
    debug_ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_ci.pfnUserCallback = VK_debug_callback;
    debug_ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debug_ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    VkInstance instance = create_instance(pAllocator, app.title, app.title, VK_API_VERSION_1_2, app.version, app.version, used_instance_extensions, used_instance_extension_count, used_instance_layers, used_instance_layer_count, &debug_ci);
    volkLoadInstanceOnly(instance);

    VkDebugUtilsMessengerEXT messenger;
    VKCHECK(vkCreateDebugUtilsMessengerEXT(instance, &debug_ci, pAllocator, &messenger));

    VkPhysicalDevice physical_devices[16] = ZERO_INIT;
    u32 physical_device_count = array_length(physical_devices);
    VKCHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));
    VkPhysicalDevice pd = pick_physical_device(physical_devices, physical_device_count);

    // Device extensions
    VkExtensionProperties device_extensions[1024];
    u32 device_extension_count;
    const char* used_device_extensions[1024];
    u32 used_device_extension_count = 0;
    VKCHECK(vkEnumerateDeviceExtensionProperties(pd, null, &device_extension_count, null));
    VKCHECK(vkEnumerateDeviceExtensionProperties(pd, null, &device_extension_count, device_extensions));
    bool swapchain_extension_found = false;
    //print("Device extension count: %u\n", device_extension_count);
    for (u32 i = 0; i < device_extension_count; i++)
    {
        //print("%s\n", device_extensions[i].extensionName);
        if (strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName) == 0)
        {
            print("Found swapchain device extension\n");
            used_device_extensions[used_device_extension_count++] = device_extensions[i].extensionName;
            swapchain_extension_found = true;
            continue;
        }
    }

    redassert(swapchain_extension_found);

    VkSurfaceKHR surface;
    VKCHECK(glfwCreateWindowSurface(instance, window, pAllocator, &surface));
    redassert(surface);

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
        VkQueueFamilyProperties queue_family = device_queue_families[i];

        VkBool32 present_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &present_support);
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT && present_support)
        {
            queue_family_index = i;
            the_queue_family = queue_family;
        }
    }

    redassert(queue_family_index != UINT32_MAX);

    VkDevice device = create_device(pAllocator, pd, used_device_extensions, used_device_extension_count, &queue_family_index, 1);
    volkLoadDevice(device);

    VkSurfaceFormatKHR surface_formats[100];
    u32 surface_format_count;
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &surface_format_count, null));
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &surface_format_count, surface_formats));
    VkSurfaceFormatKHR surface_format = get_swapchain_format(surface_formats, surface_format_count);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VKCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &surface_capabilities));
    VkExtent2D extent = surface_capabilities.currentExtent;

    VkPresentModeKHR present_modes[64];
    u32 present_mode_count;
    VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &present_mode_count, null));
    VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &present_mode_count, present_modes));
    for (u32 i = 0; i < present_mode_count; i++)
    {
        VkPresentModeKHR present_mode = present_modes[i];
        print("Present mode: %s\n", present_mode_string(present_mode));
    }

    VkSwapchainKHR swapchain = create_swapchain(pAllocator, device, surface, surface_capabilities, surface_format, extent, queue_family_index);
    redassert(swapchain);

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
        redassert(swapchain_image_views[i]);
    }

    VkQueue queue;
    u32 queue_index = 0;
    vkGetDeviceQueue(device, queue_family_index, queue_index, &queue);
    redassert(queue);

    VkCommandPool command_pool;
    VkCommandPoolCreateInfo command_pool_ci =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queue_family_index,
    };
    VKCHECK(vkCreateCommandPool(device, &command_pool_ci, pAllocator, &command_pool));
    redassert(command_pool);

    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo command_buffer_ai =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .commandBufferCount = 1,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };

    VKCHECK(vkAllocateCommandBuffers(device, &command_buffer_ai, &command_buffer));

    VkAttachmentDescription color_attachment =
    {
        .format = surface_format.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref =
    {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass_desc =
    {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
    };

    VkRenderPassCreateInfo rp_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pAttachments = &color_attachment,
        .attachmentCount = 1,
        .pSubpasses = &subpass_desc,
        .subpassCount = 1,
    };

    VkRenderPass render_pass;
    VKCHECK(vkCreateRenderPass(device, &rp_create_info, pAllocator, &render_pass));

    const char* shader_filenames[] =
    {
        "trianglev.spv",
        "trianglef.spv",
    };
    VkShaderStageFlagBits hardcoded_shader_stages[array_length(shader_filenames)] =
    {
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkPipelineShaderStageCreateInfo shader_stages[array_length(shader_filenames)];
    u32 shader_count = array_length(shader_filenames);

    for (u32 i = 0; i < array_length(shader_filenames); i++)
    {
        SB* file = os_file_load(shader_filenames[i]);
        redassert(file);
        u32 spirv_byte_count = file->len - 1;
        redassert(spirv_byte_count % sizeof(u32) == 0);

        const u32* spirv_code_ptr = (const u32*)file->ptr;
        // 
        u32 spirv_code_size = spirv_byte_count;

        VkShaderStageFlagBits shader_stage = hardcoded_shader_stages[i];
        redassert(shader_stage);

        VkShaderModuleCreateInfo ci = 
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pCode = spirv_code_ptr,
            .codeSize = spirv_code_size,
        };

        VkShaderModule shader_module;
        VKCHECK(vkCreateShaderModule(device, &ci, pAllocator, &shader_module));

        shader_stages[i] = pipeline_shader_stage_create_info(shader_stage, shader_module);
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_ci =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_ci = pipeline_input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    VkPipelineRasterizationStateCreateInfo rasterization_state_ci = rasterization_state_create_info(VK_POLYGON_MODE_FILL);

    VkPipelineMultisampleStateCreateInfo multisample_state_ci =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_state_ci =
    {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_ci =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };


    VkPipelineColorBlendStateCreateInfo color_blend_state_ci =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state_ci,
    };
    print("Color blend\n");

    VkViewport viewport =
    {
        .x = 0.0f,
        .y = 0.0f,
        .width = (f32) extent.width,
        .height = (f32) extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    print("Viewport\n");

    VkRect2D scissor =
    {
        .extent = extent,
    };
    print("Scissor\n");

    VkPipelineViewportStateCreateInfo viewport_state_ci =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };
    print("Viewport state\n");

    VkPipelineLayout pipeline_layout;
    VKCHECK(vkCreatePipelineLayout(device, &pipeline_layout_ci, pAllocator, &pipeline_layout));

    VkGraphicsPipelineCreateInfo graphics_pipeline_ci =
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = shader_count,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_state_ci,
        .pInputAssemblyState = &input_assembly_ci,
        .pViewportState = &viewport_state_ci,
        .pRasterizationState = &rasterization_state_ci,
        .pMultisampleState = &multisample_state_ci, 
        .pColorBlendState = &color_blend_state_ci,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
    };
    
    print("Reached before pipeline creation\n");
    VkPipeline graphics_pipeline;
    VKCHECK(vkCreateGraphicsPipelines(device, null, 1, &graphics_pipeline_ci, pAllocator, &graphics_pipeline));
    print("Created graphics pipeline\n");


    VkFramebufferCreateInfo fb_create_info =
    {
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
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VkFence fence;
    VKCHECK(vkCreateFence(device, &fence_create_info, pAllocator, &fence));

    VkSemaphoreCreateInfo sem_create_info =
    {
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
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        VKCHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

        VkClearValue clear_value = ZERO_INIT;
        f32 flash = (f32)fabs(sin(frame_number / 120.f));
        clear_value.color = (VkClearColorValue) { { 0.0f, 0.0f, flash, 1.0f} };

        VkRenderPassBeginInfo rp_begin_info =
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass,
            .renderArea.extent = extent,
            .framebuffer = framebuffers[swapchain_image_index],
            .pClearValues = &clear_value,
            .clearValueCount = 1,
        };

        vkCmdBeginRenderPass(command_buffer, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        /***** BEGIN RENDER ******/
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
        vkCmdDraw(command_buffer, 3, 1, 0, 0);
        /***** END RENDER ******/

        vkCmdEndRenderPass(command_buffer);
        VKCHECK(vkEndCommandBuffer(command_buffer));

        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info =
        {
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
