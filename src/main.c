#include "types.h"
#include "os.h"
#include "maths.h"
#include "dependencies/volk.h"
#include "dependencies/vk_mem_alloc.h"
#include "dependencies/fast_obj.h"
#include <meshoptimizer.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <math.h>
#include <vulkan/vulkan_core.h>
#ifdef RED_OS_WINDOWS
#include <spirv-headers/spirv.h>
#else
#include <spirv/1.2/spirv.h>
#endif

static u32 space_selector = 0;

typedef struct Vertex
{
    f32 position[3];
    f32 normal[3];
    f32 color[3];
} Vertex;

GEN_BUFFER_STRUCT(VkVertexInputBindingDescription)
GEN_BUFFER_STRUCT(VkVertexInputAttributeDescription)
GEN_BUFFER_FUNCTIONS(vertex_binding, vb, VkVertexInputBindingDescriptionBuffer, VkVertexInputBindingDescription)
GEN_BUFFER_FUNCTIONS(vertex_attribute, vab, VkVertexInputAttributeDescriptionBuffer, VkVertexInputAttributeDescription)

typedef struct VertexInputDescription
{
    VkVertexInputBindingDescriptionBuffer bindings;
    VkVertexInputAttributeDescriptionBuffer attributes;

    VkPipelineVertexInputStateCreateFlags flags;
} VertexInputDescription;

GEN_BUFFER_STRUCT(Vertex)
GEN_BUFFER_FUNCTIONS(vertices, vb, VertexBuffer, Vertex)

typedef struct AllocatedBuffer
{
    VkBuffer handle;
    VmaAllocation allocation;
} AllocatedBuffer;

typedef struct AllocatedImage
{
    VkImage handle;
    VmaAllocation allocation;
} AllocatedImage;

typedef struct MeshPushConstants
{
    vec4f data;
    mat4f render_matrix;
} MeshPushConstants;

typedef struct Mesh
{
    VertexBuffer vertices;
    AllocatedBuffer buffer;
} Mesh;

typedef struct Material
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
} Material;

const f32 yaw = -90.0f;
const f32 pitch = 0.0f;
const f32 speed = 0.10f;
const f32 sensitivity = 0.1;
const f32 zoom = 45.0f;

typedef struct Camera
{
    vec3f position;
    vec3f front;
    vec3f right;
    vec3f up;
    f32 movement_speed;
    f32 last_x, last_y;
    f32 yaw;
    f32 pitch;
    f32 mouse_sensitivity;
    f32 zoom;
} Camera;

typedef enum CameraMovement
{
    CAMERA_MOVEMENT_FORWARD,
    CAMERA_MOVEMENT_BACKWARD,
    CAMERA_MOVEMENT_LEFT,
    CAMERA_MOVEMENT_RIGHT,
} CameraMovement;

static inline void Camera_process_key(Camera* camera, CameraMovement movement, f32 delta_time)
{
    f32 velocity = camera->movement_speed * delta_time;
    vec3f position = camera->position;
    switch (movement)
    {
        case CAMERA_MOVEMENT_BACKWARD:
        {
            print("Camera backward\n");
            vec3f front = camera->front;
            camera->position = vec3_sub(position, vec3_scale(front, velocity));
            break;
        }
        case CAMERA_MOVEMENT_FORWARD:
        {
            print("Camera forward\n");
            vec3f front = camera->front;
            camera->position = vec3_add(position, vec3_scale(front, velocity));
            break;
        }
        case CAMERA_MOVEMENT_LEFT:
        {
            print("Camera left\n");
            vec3f right = camera->right;
            camera->position = vec3_sub(position, vec3_scale(right, velocity));
            break;
        }
        case CAMERA_MOVEMENT_RIGHT:
        {
            print("Camera right\n");
            vec3f right = camera->right;
            camera->position = vec3_add(position, vec3_scale(right, velocity));
            break;
        }
        default:
            RED_UNREACHABLE;
            break;
    }
}

typedef struct Application
{
    struct
    {
        union
        {
            GLFWwindow* glfw;
        } handle;
        s32 width, height;
    } window;
    char* title;
    u32 version;
    Camera camera;
    f32 delta_time;
    bool first_mouse;
} Application;

#define VKCHECK(_result) do { VkResult result = _result; if (result == VK_SUCCESS) { /*print("%s OK\n", #_result);*/ } else { print("%s FAIL! %s\n", #_result, VKResultToString(_result)); redassert(result == VK_SUCCESS); } } while(false);

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
    VkApplicationInfo application_info =
    {
        .apiVersion = api_version,
        .engineVersion = engine_version,
        .applicationVersion = application_version,
        .pApplicationName = application_name,
        .pEngineName = engine_name,
        .pNext = NULL,
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .applicationVersion = 1,
    };

    VkInstanceCreateInfo instance_ci =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = debug_utils,
        .pApplicationInfo = &application_info,
        .ppEnabledExtensionNames = extension_count > 0 ? extensions : NULL,
        .enabledExtensionCount = extension_count,
        .ppEnabledLayerNames = layer_count > 0 ? layers : NULL,
        .enabledLayerCount = layer_count,
    };

    VkInstance instance;
    VKCHECK(vkCreateInstance(&instance_ci, pAllocator, &instance));

    return instance;
}

static VkBool32 VK_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    print("%s\n", pCallbackData->pMessage);
    redassert(!(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT));

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
        queue_create_infos[i] = (VkDeviceQueueCreateInfo)
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queue_family_index,
                .queueCount = 1,
                .pQueuePriorities = queue_priorities,
        };
    }

    VkDeviceCreateInfo device_ci =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .ppEnabledExtensionNames = device_extension_count > 0 ? device_extensions : NULL,
        .enabledExtensionCount = device_extension_count,
        .pQueueCreateInfos = queue_create_infos,
        .queueCreateInfoCount = queue_family_count,
    };

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


    VkSwapchainCreateInfoKHR swapchain_ci =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = MAX(2, surface_capabilities.minImageCount),
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = surface_composite,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .pQueueFamilyIndices = &queue_family_index,
        .queueFamilyIndexCount = 1,
    };

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

typedef struct ShaderProgram
{
    const char* shaders[2];
    bool vertex_buffer;
} ShaderProgram;

typedef struct ShaderProgramVK
{
    VkPipelineShaderStageCreateInfo shader_stages[2];
} ShaderProgramVK;

VertexInputDescription Vertex_get_description(void)
{
    VertexInputDescription description = ZERO_INIT;

    VkVertexInputBindingDescription main_binding =
    {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    vertex_binding_append(&description.bindings, main_binding);

    VkVertexInputAttributeDescription position_attribute =
    {
        .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, position),
    };
    VkVertexInputAttributeDescription normal_attribute =
    {
        .binding = 0,
        .location = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, normal),
    };
    VkVertexInputAttributeDescription color_attribute =
    {
        .binding = 0,
        .location = 2,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, color),
    };

    vertex_attribute_append(&description.attributes, position_attribute);
    vertex_attribute_append(&description.attributes, normal_attribute);
    vertex_attribute_append(&description.attributes, color_attribute);

    return description;
}

static inline VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlagBits usage_flags, VkExtent3D extent)
{
    VkImageCreateInfo image_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage_flags,
    };

    return image_create_info;
}

static inline VkImageViewCreateInfo image_view_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags)
{
    VkImageViewCreateInfo image_view_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .image = image,
        .format = format,
        .subresourceRange =
        {
            .levelCount = 1,
            .layerCount = 1,
            .aspectMask = aspect_flags,
        },
    };

    return image_view_create_info;
}

static inline VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool depth_test, bool depth_write, VkCompareOp compare_op)
{
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_ci =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = depth_test,
        .depthWriteEnable = depth_write,
        .depthCompareOp = depth_test ? compare_op : VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = false,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
        .stencilTestEnable = false,
    };

    return depth_stencil_state_ci;
}

static Mesh mesh_load(const char* path)
{
    fastObjMesh* obj = fast_obj_read(path);
    redassert(obj);
    u64 index_count = 0;
    u32 face_count = obj->face_count;

    for (u32 i = 0; i < face_count; i++)
    {
        index_count += 3 * (obj->face_vertices[i] - 2);
    }

    VertexBuffer vb = ZERO_INIT;
    vertices_resize(&vb, index_count);
    vb.len = index_count;

    u64 vertex_offset = 0;
    u64 index_offset = 0;

    for (u32 i = 0; i < face_count; i++)
    {
        u32 face_vertices = obj->face_vertices[i];
        for (u32 j = 0; j < face_vertices; j++)
        {
            fastObjIndex gi = obj->indices[index_offset + j];

            if (j >= 3)
            {
                vb.ptr[vertex_offset + 0] = vb.ptr[vertex_offset - 3];
                vb.ptr[vertex_offset + 1] = vb.ptr[vertex_offset - 1];
                vertex_offset += 2;
            }

            vb.ptr[vertex_offset++] = (Vertex)
            {
                .position[0] = obj->positions[gi.p * 3 + 0],
                .position[1] = obj->positions[gi.p * 3 + 1],
                .position[2] = obj->positions[gi.p * 3 + 2],

                .normal[0] = obj->normals[gi.n * 3 + 0],
                .normal[1] = obj->normals[gi.n * 3 + 1],
                .normal[2] = obj->normals[gi.n * 3 + 2],

                .color[0] = obj->normals[gi.n * 3 + 0],
                .color[1] = obj->normals[gi.n * 3 + 1],
                .color[2] = obj->normals[gi.n * 3 + 2],
            };
        }

        index_offset += obj->face_vertices[i];
    }

    redassert(vertex_offset == index_count);
    fast_obj_destroy(obj);

    Mesh mesh = ZERO_INIT;
    /*u32* remap = NEW(u32, index_count);*/
    /*u64 vertex_count = meshopt_generateVertexRemap(remap, NULL, index_count, vb.ptr, index_count, sizeof(Vertex));*/

    /*vertices_resize(&mesh.vertices, vertex_count);*/
    /*mesh.vertices.len = vertex_count;*/

    /*meshopt_remapVertexBuffer(mesh.vertices.ptr, vb.ptr, , size_t vertex_size, const unsigned int *remap)*/
    /*meshopt_remapIndexBuffer(m.indices, NULL, indexCount, remap);*/
    mesh.vertices = vb;
    return mesh;
}

static void glfw_mouse_callback(GLFWwindow* window, double x, double y)
{
    Application* app = (Application*) glfwGetWindowUserPointer(window);
    if (app->first_mouse)
    {
        app->camera.last_x = x;
        app->camera.last_y = y;
        app->first_mouse = false;
    }

    f32 x_offset = (x - app->camera.last_x) * app->camera.mouse_sensitivity;
    f32 y_offset = (y - app->camera.last_y) * app->camera.mouse_sensitivity;

    app->camera.last_x = x;
    app->camera.last_y = y;

    app->camera.yaw += x_offset;
    app->camera.pitch -= y_offset;
}

static inline mat4f Camera_update_view(Camera* camera)
{
    camera->pitch = MIN(camera->pitch, 89.0f);
    camera->pitch = MAX(camera->pitch, -89.0f);

    camera->front.x = -sinf(rad(camera->yaw)) * cosf(rad(camera->pitch));
    camera->front.y = sinf(rad(camera->pitch));
    camera->front.z = cosf(rad(camera->yaw)) * cosf(rad(camera->pitch));

    camera->front = vec3_normalize(camera->front);
    camera->right = vec3_normalize(vec3_cross(VEC3(0, 1.f, 0), camera->front));
    camera->up = vec3_normalize(vec3_cross(camera->front, camera->right));

    mat4f view_mat = lookat(camera->position, vec3_add(camera->position, camera->front), camera->up);
    mat4f_print(view_mat);
    return view_mat;
}

static inline void Camera_process_scroll(Camera* camera, f32 offset)
{
    camera->zoom -= offset;
    if (camera->zoom < 1.0f)
    {
        camera->zoom = 1.0f;
    }
    if (camera->zoom > 45.0f)
    {
        camera->zoom = 45.0f;
    }
}

static void glfw_scroll_callback(GLFWwindow* window, double x, double y)
{
    Application* app = (Application*) glfwGetWindowUserPointer(window);
    Camera_process_scroll(&app->camera, y);
}

static void app_handle_input(Application* app)
{
    f32 camera_speed = app->camera.movement_speed * app->delta_time;

    GLFWwindow* window = app->window.handle.glfw;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
        return;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        app->camera.position = vec3_add(app->camera.position, vec3_scale(app->camera.front, camera_speed));
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        app->camera.position = vec3_sub(app->camera.position, vec3_scale(app->camera.front, camera_speed));
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        app->camera.position = vec3_add(app->camera.position, vec3_scale(app->camera.right, camera_speed));
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        app->camera.position = vec3_sub(app->camera.position, vec3_scale(app->camera.right, camera_speed));
    }
}

static inline Camera Camera_init(void)
{
    Camera camera =
    {
        .position = VEC3(0.0f, 0.0f, -20.0f),
        .yaw = 0.0f,
        .pitch = 0.0f,
        .movement_speed = speed,
        .mouse_sensitivity = sensitivity,
        .zoom = zoom,
    };
    return camera;
}

typedef struct Frame
{
    struct
    {
        VkSemaphore present_sem, render_sem;
        VkFence render_fence;
        VkCommandPool command_pool;
        VkCommandBuffer command_buffer;
    } sync;

    AllocatedBuffer camera_buffer;
    VkDescriptorSet global_descriptor;
} Frame;

#define FRAME_OVERLAP (2)
const u32 frame_overlap = FRAME_OVERLAP;

static inline AllocatedBuffer create_buffer(VmaAllocator allocator, usize allocation_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage)
{
    VkBufferCreateInfo ci = 
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = allocation_size,
        .usage = usage,
    };

    VmaAllocationCreateInfo ai =
    {
        .usage = memory_usage,
    };

    AllocatedBuffer buffer;

    VKCHECK(vmaCreateBuffer(allocator, &ci, &ai, &buffer.handle, &buffer.allocation, null));

    return buffer;
}

s32 main(s32 argc, char* argv[])
{
    os_init();

    Application app =
    {
        .title = "Hello world",
        .window.width = 1024,
        .window.height = 768, 
        .version = 1,
        .delta_time = 0,
        .camera = Camera_init(),
        .first_mouse = true,
    };

    Frame frame[FRAME_OVERLAP] = ZERO_INIT;

    s32 result = glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    volkInitialize();
    redassert(result == GLFW_TRUE);
    app.window.handle.glfw = glfwCreateWindow(app.window.width, app.window.height, app.title, NULL, NULL);
    redassert(app.window.handle.glfw);
    glfwSetWindowUserPointer(app.window.handle.glfw, &app);
    glfwSetCursorPosCallback(app.window.handle.glfw, glfw_mouse_callback);
    glfwSetScrollCallback(app.window.handle.glfw, glfw_scroll_callback);

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

    VkDebugUtilsMessengerCreateInfoEXT debug_ci =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pfnUserCallback = VK_debug_callback,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
    };

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
    VKCHECK(glfwCreateWindowSurface(instance, app.window.handle.glfw, pAllocator, &surface));
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
    VmaVulkanFunctions vma_f =
    {
        .vkGetPhysicalDeviceProperties=          vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties=    vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory=                       vkAllocateMemory,
        .vkFreeMemory=                           vkFreeMemory,
        .vkMapMemory=                            vkMapMemory,
        .vkUnmapMemory=                          vkUnmapMemory,
        .vkFlushMappedMemoryRanges=              vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges=         vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory=                     vkBindBufferMemory,
        .vkBindImageMemory=                      vkBindImageMemory,
        .vkGetBufferMemoryRequirements=          vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements=           vkGetImageMemoryRequirements,
        .vkCreateBuffer=                         vkCreateBuffer,
        .vkDestroyBuffer=                        vkDestroyBuffer,
        .vkCreateImage=                          vkCreateImage,
        .vkDestroyImage=                         vkDestroyImage,
        .vkCmdCopyBuffer=                        vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR=      vkGetBufferMemoryRequirements2KHR,
        .vkGetImageMemoryRequirements2KHR=       vkGetImageMemoryRequirements2KHR,
        .vkBindBufferMemory2KHR=                 vkBindBufferMemory2KHR,
        .vkBindImageMemory2KHR=                  vkBindImageMemory2KHR,
        .vkGetPhysicalDeviceMemoryProperties2KHR=vkGetPhysicalDeviceMemoryProperties2KHR,
    };

    VmaAllocatorCreateInfo allocator_ci =
    {
        .physicalDevice = pd,
        .device = device,
        .instance = instance,
        .pVulkanFunctions = &vma_f,
    };

    VmaAllocator allocator;
    VKCHECK(vmaCreateAllocator(&allocator_ci, &allocator));

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
    VkFormat depth_format = VK_FORMAT_D32_SFLOAT;

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

    VkAttachmentDescription depth_attachment = 
    {
        .format = depth_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depth_attachment_ref =
    {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass_desc =
    {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        .pDepthStencilAttachment = &depth_attachment_ref,
    };

    VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };

    VkRenderPassCreateInfo rp_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pAttachments = attachments,
        .attachmentCount = array_length(attachments),
        .pSubpasses = &subpass_desc,
        .subpassCount = 1,
    };

    VkRenderPass render_pass;
    VKCHECK(vkCreateRenderPass(device, &rp_create_info, pAllocator, &render_pass));

    VkExtent3D depth_extent = 
    {
        .width = extent.width,
        .height = extent.height,
        .depth = 1,
    };

    VkImageCreateInfo depth_image_ci = image_create_info(depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depth_extent);

    VmaAllocationCreateInfo depth_image_ai =
    {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    AllocatedImage depth_image = ZERO_INIT;

    VKCHECK(vmaCreateImage(allocator, &depth_image_ci, &depth_image_ai, &depth_image.handle, &depth_image.allocation, null));

    VkImageViewCreateInfo depth_image_view_ci = image_view_create_info(depth_format, depth_image.handle, VK_IMAGE_ASPECT_DEPTH_BIT);
    VkImageView depth_image_view;
    VKCHECK(vkCreateImageView(device, &depth_image_view_ci, pAllocator, &depth_image_view));

    VkImage swapchain_images[4];
    u32 swapchain_image_count;
    VKCHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, null));
    redassert(swapchain_image_count <= array_length(swapchain_images));
    VKCHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images));

    VkImageView swapchain_image_views[array_length(swapchain_images)];
    VkFramebuffer framebuffers[array_length(swapchain_images)];

    VkImageViewCreateInfo swapchain_image_view_ci =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .components.r = VK_COMPONENT_SWIZZLE_R,
        .components.g = VK_COMPONENT_SWIZZLE_G,
        .components.b = VK_COMPONENT_SWIZZLE_B,
        .components.a = VK_COMPONENT_SWIZZLE_A,
        .format = surface_format.format,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.layerCount = 1,
        .subresourceRange.levelCount = 1,
    };

    VkFramebufferCreateInfo fb_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = 1,
        .width = extent.width,
        .height = extent.height,
        .layers = 1,
    };

    for (u32 i = 0; i < swapchain_image_count; i++)
    {
        swapchain_image_view_ci.image = swapchain_images[i];
        VKCHECK(vkCreateImageView(device, &swapchain_image_view_ci, pAllocator, &swapchain_image_views[i]));
        redassert(swapchain_image_views[i]);
        VkImageView attachments[] = { swapchain_image_views[i], depth_image_view };
        fb_create_info.pAttachments = attachments;
        fb_create_info.attachmentCount = array_length(attachments);
        VKCHECK(vkCreateFramebuffer(device, &fb_create_info, pAllocator, &framebuffers[i]));
    }

#define MESH_PIPELINE_INDEX 0
    const u32 mesh_pipeline_index = MESH_PIPELINE_INDEX;
    ShaderProgram shader_programs[] =
    {
        [MESH_PIPELINE_INDEX] =
        {
            .shaders[0] = "triangle_meshv.spv",
            .shaders[1] = "triangle_meshf.spv",
            .vertex_buffer = true,
        },
    };

    VkShaderStageFlagBits hardcoded_shader_stages[] =
    {
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    u32 pipeline_count = array_length(shader_programs);
    u32 shader_program_stage_count = array_length(hardcoded_shader_stages);
    VkPipelineShaderStageCreateInfo program_shaders[array_length(hardcoded_shader_stages)];
    ShaderProgramVK pipeline_shaders_create_info[array_length(shader_programs)];
    VkGraphicsPipelineCreateInfo graphics_pipelines_create_info[array_length(shader_programs)] = ZERO_INIT;

    for (u32 i = 0; i < pipeline_count; i++)
    {
        ShaderProgram* shader_program = &shader_programs[i];

        for (u32 shader_stage_index = 0; shader_stage_index < shader_program_stage_count; shader_stage_index++)
        {
            SB* file = os_file_load(shader_program->shaders[shader_stage_index]);
            redassert(file);
            u32 spirv_byte_count = file->len - 1;
            redassert(spirv_byte_count % sizeof(u32) == 0);

            const u32* spirv_code_ptr = (const u32*)file->ptr;
            u32 spirv_code_size = spirv_byte_count;

            VkShaderStageFlagBits shader_stage = hardcoded_shader_stages[shader_stage_index];
            redassert(shader_stage);

            VkShaderModuleCreateInfo ci = 
            {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pCode = spirv_code_ptr,
                .codeSize = spirv_code_size,
            };

            VkShaderModule shader_module;
            VKCHECK(vkCreateShaderModule(device, &ci, pAllocator, &shader_module));

            pipeline_shaders_create_info[i].shader_stages[shader_stage_index] = pipeline_shader_stage_create_info(shader_stage, shader_module);
        }

        VkPipelineVertexInputStateCreateInfo vertex_input_state_ci =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        };

        if (shader_program->vertex_buffer)
        {
            VertexInputDescription desc = Vertex_get_description();
            vertex_input_state_ci.pVertexAttributeDescriptions = desc.attributes.ptr;
            vertex_input_state_ci.vertexAttributeDescriptionCount = desc.attributes.len;
            vertex_input_state_ci.pVertexBindingDescriptions = desc.bindings.ptr;
            vertex_input_state_ci.vertexBindingDescriptionCount = desc.bindings.len;
        }

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

        VkPipelineColorBlendStateCreateInfo color_blend_state_ci =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &color_blend_attachment_state_ci,
        };

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state_ci = depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

        VkViewport viewport =
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = (f32) extent.width,
            .height = (f32) extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        VkRect2D scissor =
        {
            .extent = extent,
        };

        VkPipelineViewportStateCreateInfo viewport_state_ci =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor,
        };

        VkPushConstantRange push_constant =
        {
            .offset = 0,
            .size = sizeof(MeshPushConstants),
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };

        VkPipelineLayoutCreateInfo pipeline_layout_ci =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pPushConstantRanges = &push_constant,
            .pushConstantRangeCount = 1,
        };

        VkPipelineLayout pipeline_layout;
        VKCHECK(vkCreatePipelineLayout(device, &pipeline_layout_ci, pAllocator, &pipeline_layout));

        graphics_pipelines_create_info[i] = (VkGraphicsPipelineCreateInfo) 
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .stageCount = shader_program_stage_count,
                .pStages = pipeline_shaders_create_info[i].shader_stages,
                .pVertexInputState = &vertex_input_state_ci,
                .pInputAssemblyState = &input_assembly_ci,
                .pViewportState = &viewport_state_ci,
                .pRasterizationState = &rasterization_state_ci,
                .pMultisampleState = &multisample_state_ci, 
                .pColorBlendState = &color_blend_state_ci,
                .pDepthStencilState = &depth_stencil_state_ci,
                .layout = pipeline_layout,
                .renderPass = render_pass,
                .subpass = 0,
        };
    }

    VkPipeline graphics_pipelines[array_length(shader_programs)];
    VKCHECK(vkCreateGraphicsPipelines(device, null, pipeline_count, graphics_pipelines_create_info, pAllocator, graphics_pipelines));

    Material materials[array_length(graphics_pipelines)];
    u32 material_count = array_length(materials);

    for (u32 i = 0; i < material_count; i++)
    {
        materials[i].pipeline = graphics_pipelines[i];
        materials[i].layout = graphics_pipelines_create_info[i].layout;
    }

    VkQueue queue;
    u32 queue_index = 0;
    vkGetDeviceQueue(device, queue_family_index, queue_index, &queue);
    redassert(queue);

    VkCommandPoolCreateInfo command_pool_ci =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queue_family_index,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };

    for (u32 i = 0; i < frame_overlap; i++)
    {
        VKCHECK(vkCreateCommandPool(device, &command_pool_ci, pAllocator, &frame[i].sync.command_pool));
        redassert(frame[i].sync.command_pool);
        VkCommandBufferAllocateInfo command_buffer_ai =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = frame[i].sync.command_pool,
            .commandBufferCount = 1,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        };

        VKCHECK(vkAllocateCommandBuffers(device, &command_buffer_ai, &frame[i].sync.command_buffer));

        VkFenceCreateInfo fence_create_info =
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        VKCHECK(vkCreateFence(device, &fence_create_info, pAllocator, &frame[i].sync.render_fence));
        VkSemaphoreCreateInfo sem_create_info =
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VKCHECK(vkCreateSemaphore(device, &sem_create_info, pAllocator, &frame[i].sync.render_sem));
        VKCHECK(vkCreateSemaphore(device, &sem_create_info, pAllocator, &frame[i].sync.present_sem));
    }

    for (u32 i = 0; i < frame_overlap; i++)
    {
    }

    Mesh monkey_mesh = mesh_load("../assets/monkey_flat.obj");
    Mesh mario_mesh = mesh_load("../assets/mario.obj");
    Mesh meshes[] = { mario_mesh };
    u32 mesh_count = array_length(meshes);

    for (u32 i = 0; i < mesh_count; i++)
    {
        Mesh* mesh = &meshes[i];
        mesh->buffer = create_buffer(allocator, mesh->vertices.len * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        void* data;
        VKCHECK(vmaMapMemory(allocator, mesh->buffer.allocation, &data));

        memcpy(data, vertices_ptr(&mesh->vertices), vertices_len(&mesh->vertices) * sizeof(Vertex));

        vmaUnmapMemory(allocator, mesh->buffer.allocation);
    }

    mat4f model_matrices[array_length(graphics_pipelines) * array_length(meshes)];
    u32 frame_number = 0;

    u64 frame_counter = os_performance_counter();
    while (!glfwWindowShouldClose(app.window.handle.glfw))
    {
        u64 internal_frame_counter = os_performance_counter();
        app.delta_time = os_compute_ms(frame_counter, internal_frame_counter);
        //print("PREVIOUS: %u, CURRENT: %u, DELTA: %.02f ms.\n", frame_counter, internal_frame_counter, app.delta_time);
        frame_counter = internal_frame_counter;
        const u32 frame_index = frame_number % frame_overlap;
        
        glfwPollEvents();

        app_handle_input(&app);

        for (u32 material_index = 0; material_index < material_count; material_index++)
        {
            for (u32 mesh_index = 0; mesh_index < mesh_count; mesh_index++)
            {
                mat4f model = rotate(MAT4_IDENTITY_INIT, rad(frame_number * 0.4f), (vec3f) {0,1,0});
                u32 index = (material_index * mesh_count) + mesh_index;
                model_matrices[index] = model;
            }
        }

        mat4f proj = perspective(rad(app.camera.zoom), (f32)app.window.width / (f32)app.window.height, 0.1, 100.0f);
        proj.row[1].v[1] *= -1;
        mat4f view = Camera_update_view(&app.camera);
        mat4f proj_x_view = mat4f_mul(proj, view);

        VKCHECK(vkWaitForFences(device, 1, &frame[frame_index].sync.render_fence, true, 1000 * 1000 * 1000));
        VKCHECK(vkResetFences(device, 1, &frame[frame_index].sync.render_fence));

        u32 swapchain_image_index;
        VKCHECK(vkAcquireNextImageKHR(device, swapchain, 0, frame[frame_index].sync.present_sem, null, &swapchain_image_index));

        VKCHECK(vkResetCommandBuffer(frame[frame_index].sync.command_buffer, 0));

        VkCommandBufferBeginInfo begin_info =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        VKCHECK(vkBeginCommandBuffer(frame[frame_index].sync.command_buffer, &begin_info));

        VkClearValue color_clear = ZERO_INIT;
        f32 flash = (f32)fabs(sin(frame_number / 120.f));
        color_clear.color = (VkClearColorValue) { { 0.0f, 0.0f, 0.0f, 1.0f} };

        VkClearValue depth_clear =
        {
            .depthStencil.depth = 1.f,
        };

        VkClearValue clear_values[] = { color_clear, depth_clear };

        VkRenderPassBeginInfo rp_begin_info =
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass,
            .renderArea.extent = extent,
            .framebuffer = framebuffers[swapchain_image_index],
            .pClearValues = clear_values,
            .clearValueCount = array_length(clear_values),
        };

        vkCmdBeginRenderPass(frame[frame_index].sync.command_buffer, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        /***** BEGIN RENDER ******/

        for (u32 material_index = 0; material_index < material_count; material_index++)
        {
            vkCmdBindPipeline(frame[frame_index].sync.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, materials[material_index].pipeline);
            for (u32 mesh_index = 0; mesh_index < mesh_count; mesh_index++)
            {
                const VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(frame[frame_index].sync.command_buffer, 0, 1, &meshes[mesh_index].buffer.handle, &offset);
                u32 model_matrix_index = (material_index * mesh_count) + mesh_index;
                MeshPushConstants constants =
                {
                    .render_matrix = mat4f_mul(proj_x_view, model_matrices[model_matrix_index]),
                };
                vkCmdPushConstants(frame[frame_index].sync.command_buffer, materials[material_index].layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
                vkCmdDraw(frame[frame_index].sync.command_buffer, meshes[mesh_index].vertices.len, 1, 0, 0);
            }
        }

        /***** END RENDER ******/

        vkCmdEndRenderPass(frame[frame_index].sync.command_buffer);
        VKCHECK(vkEndCommandBuffer(frame[frame_index].sync.command_buffer));

        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info =
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pWaitDstStageMask = &wait_stage,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frame[frame_index].sync.present_sem,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &frame[frame_index].sync.render_sem,
            .commandBufferCount = 1,
            .pCommandBuffers = &frame[frame_index].sync.command_buffer,
        };

        VKCHECK(vkQueueSubmit(queue, 1, &submit_info, frame[frame_index].sync.render_fence));

        VkPresentInfoKHR present_info =
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pSwapchains = &swapchain,
            .swapchainCount = 1,
            .pWaitSemaphores = &frame[frame_index].sync.render_sem,
            .waitSemaphoreCount = 1,
            .pImageIndices = &swapchain_image_index,
        };

        VKCHECK(vkQueuePresentKHR(queue, &present_info));

        frame_number++;

        //return 0;
    }

    for (u32 i = 0; i < frame_overlap; i++)
    {
        VKCHECK(vkWaitForFences(device, 1, &frame[i].sync.render_fence, true, 1000 * 1000 * 1000));
        vkDestroyFence(device, frame[i].sync.render_fence, pAllocator);
        vkDestroySemaphore(device, frame[i].sync.render_sem, pAllocator);
        vkDestroySemaphore(device, frame[i].sync.present_sem, pAllocator);
        vkFreeCommandBuffers(device, frame[i].sync.command_pool, 1, &frame[i].sync.command_buffer);
        vkDestroyCommandPool(device, frame[i].sync.command_pool, pAllocator);
    }

    for (u32 i = 0; i < pipeline_count; i++)
    {
        u32 stage_count = graphics_pipelines_create_info[i].stageCount;
        for (u32 stage = 0; stage < stage_count; stage++)
        {
            vkDestroyShaderModule(device, graphics_pipelines_create_info[i].pStages[stage].module, pAllocator);
        }
        vkDestroyPipelineLayout(device, graphics_pipelines_create_info[i].layout, pAllocator);
        vkDestroyPipeline(device, graphics_pipelines[i], pAllocator);
    }

    vkDestroyRenderPass(device, render_pass, pAllocator);

    for (u32 i = 0; i < swapchain_image_count; i++)
    {
        vkDestroyFramebuffer(device, framebuffers[i], pAllocator);
        vkDestroyImageView(device, swapchain_image_views[i], pAllocator);
    }

    vkDestroySwapchainKHR(device, swapchain, pAllocator);
    vkDestroySurfaceKHR(instance, surface, pAllocator);
    vkDestroyDevice(device, pAllocator);
    vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
    vkDestroyInstance(instance, pAllocator);

    glfwDestroyWindow(app.window.handle.glfw);
}
