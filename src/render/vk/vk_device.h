#ifndef VK_DEVICE_H
#define VK_DEVICE_H

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
    VkQueue presentq;

    VmaAllocator allocator;
    
    bool has_rt;
public:
    VKDevice(Window& w);
    ~VKDevice();

    VkDevice get_logic_device() const { return device; }
    VkPhysicalDevice get_phys_device() const { return physical_device; }
    VkQueue get_graphicsq() const { return graphicsq; }
    VmaAllocator get_allocator() const { return allocator; }
};
#endif // VK_DEVICE_H