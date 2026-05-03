#include "vk_device.h"

VKDevice::VKDevice(Window& w){
    GLFWwindow* window = w.get_glfw_window();
    if (volkInitialize() != VK_SUCCESS) {
        throw std::runtime_error("volk init fail");
    }

    /* 1: creating instance */

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "KHORS";
    app_info.applicationVersion = VK_MAKE_VERSION(1,0,0);
    app_info.pEngineName = "KHORS Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1,0,0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    uint32_t glfw_ex_c = 0;
    const char** glfw_ex = glfwGetRequiredInstanceExtensions(&glfw_ex_c);
    create_info.enabledExtensionCount = glfw_ex_c;
    create_info.ppEnabledExtensionNames = glfw_ex;

    const char* validation_layers[] = { "VK_LAYER_KHRONOS_validation" };
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = validation_layers;

    vkCreateInstance(&create_info, nullptr, &instance);

    volkLoadInstance(instance);

    /* 2: debugger */

    // TODO

    /* 3: window surface */
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("window surface init fail");
    }

    /* 4: selecting physical device */

    physical_device = VK_NULL_HANDLE;
    uint32_t devicec = 0;
    vkEnumeratePhysicalDevices(instance, &devicec, nullptr);

    std::vector<VkPhysicalDevice> devices(devicec);
    vkEnumeratePhysicalDevices(instance, &devicec, devices.data());

    for (const auto& d : devices) {
        if (!has_required_extensions(d)) continue;
        if (!has_required_features(d)) continue;

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(d, &props);

        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physical_device = d;
            break;
        }
    }

    /* 5: queue families */

    uint32_t q_family_c = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &q_family_c, nullptr);
    std::vector<VkQueueFamilyProperties> q_families(q_family_c);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &q_family_c, q_families.data());

    uint32_t graphics_family = -1;
    uint32_t present_family = -1;
    // uint32_t compute_family = -1; might be needed later (?)
    
    for (uint32_t i = 0; i < q_family_c; ++i) {

        if (q_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_family = i;
        }

        VkBool32 is_present = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &is_present);
        if (is_present) {
            present_family = i;
        }

        // if (q_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
        //     compute_family = i;
        // }

        if (graphics_family != -1 && present_family != -1 /*&& compute_family != -1*/)
            break;
    }

    /* 6: logical device creation */

    float qpriority = 1.0f;
    VkDeviceQueueCreateInfo q_create_info{};
    q_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_create_info.queueFamilyIndex = graphics_family;
    q_create_info.queueCount = 1;
    q_create_info.pQueuePriorities = &qpriority;

    const std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,

        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,

        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
    };
    
    VkPhysicalDeviceBufferDeviceAddressFeatures bda{};
    bda.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bda.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accel{};
    accel.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accel.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipeline{};
    rt_pipeline.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rt_pipeline.rayTracingPipeline = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.bufferDeviceAddress = VK_TRUE;
    features12.descriptorIndexing = VK_TRUE;

    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;

    rt_pipeline.pNext = nullptr;
    accel.pNext = &rt_pipeline;
    bda.pNext = &accel;
    features12.pNext = &bda;
    features13.pNext = &features12;

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = &q_create_info;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pNext = &features13;
    device_create_info.pEnabledFeatures = nullptr;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames = device_extensions.data();

    vkCreateDevice(physical_device, &device_create_info, nullptr, &device);
    volkLoadDevice(device);

    /* 7: queue handles */

    vkGetDeviceQueue(device, graphics_family, 0, &graphicsq);
    vkGetDeviceQueue(device, present_family, 0, &presentq);

    /* 8: VMA */

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_info.physicalDevice = physical_device;
    allocator_info.device = device;
    allocator_info.instance = instance;
    allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; 

    VmaVulkanFunctions funcs{};
    funcs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    funcs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    allocator_info.pVulkanFunctions = &funcs;
    
    vmaCreateAllocator(&allocator_info, &allocator);

    has_rt = true;
}
VKDevice::~VKDevice(){
    if (allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(allocator);
    }
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
    }
    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
    }
}

bool VKDevice::has_required_extensions(VkPhysicalDevice device) {
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());

    std::set<std::string> required = {
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
    };

    for (const auto& ext : extensions) {
        required.erase(ext.extensionName);
    }

    return required.empty();
}
bool VKDevice::has_required_features(VkPhysicalDevice device) {
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accel{};
    accel.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt{};
    rt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rt.pNext = &accel;

    VkPhysicalDeviceBufferDeviceAddressFeatures bda{};
    bda.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bda.pNext = &rt;

    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &bda;

    vkGetPhysicalDeviceFeatures2(device, &features);

    return accel.accelerationStructure && rt.rayTracingPipeline && bda.bufferDeviceAddress;
}
