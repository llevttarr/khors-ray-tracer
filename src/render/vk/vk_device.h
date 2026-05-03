#ifndef VK_DEVICE_H
#define VK_DEVICE_H

#include <vector>
#include <stdexcept>
#include <iostream>
#include <string>
#include <set>

#include <volk.h>
#include "vk_mem_alloc.h"

#include "window.h"

/**
 * Handles the connection to the GPU
 */
class VKDevice{
private:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugger_msg;
    VkPhysicalDevice physical_device;
    VkDevice device;
    
    VkQueue graphicsq;
    // VkQueue computeq;
    VkQueue presentq;

    VmaAllocator allocator;
    VkSurfaceKHR surface;

    bool has_rt;

    bool has_required_extensions(VkPhysicalDevice device);
    bool has_required_features(VkPhysicalDevice device);
public:
    VKDevice(Window& w);
    ~VKDevice();

    VkDevice get_logic_device() const { return device; }
    VkPhysicalDevice get_phys_device() const { return physical_device; }
    VkQueue get_graphicsq() const { return graphicsq; }
    VkQueue get_presentq() const { return presentq; }
    VmaAllocator get_allocator() const { return allocator; }
    VkSurfaceKHR get_surface() const { return surface;}
};
#endif // VK_DEVICE_H