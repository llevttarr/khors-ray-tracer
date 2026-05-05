#ifndef VK_DEBUGGER_H
#define VK_DEBUGGER_H

#include <volk.h>
#include <iostream>

class VKDebugger {
private:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,void* pUserData);

public:
    VKDebugger() = default;
    ~VKDebugger();

    void init(VkInstance vk_instance);
    static void populate_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
};

#endif // VK_DEBUGGER_H