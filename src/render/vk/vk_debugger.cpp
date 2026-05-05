#include "vk_debugger.h"

VKAPI_ATTR VkBool32 VKAPI_CALL VKDebugger::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,void* pUserData) 
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "\n[VULKAN VALIDATION LAYER]\n" 
                  << "Severity: " << messageSeverity << "\n"
                  << "Message: " << pCallbackData->pMessage << "\n"
                  << "----------------------------------------\n";
    }

    return VK_FALSE;
}

void VKDebugger::populate_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                             
    createInfo.pfnUserCallback = debug_callback;
    createInfo.pUserData = nullptr;
}

void VKDebugger::init(VkInstance vk_instance) {
    instance = vk_instance;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populate_create_info(createInfo);

    if (vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debug_messenger) != VK_SUCCESS) {
        std::cerr << "Failed to set up debug messenger!" << std::endl;
    }
}

VKDebugger::~VKDebugger() {
    if (debug_messenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    }
}
