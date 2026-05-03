#include "vk_swapchain.h"

VKSwapchain::VKSwapchain(VKDevice* vkd, ProgramState& ps){
    device = std::make_shared<VKDevice>(vkd);
    init(ps.w,ps.h);
}
VKSwapchain::~VKSwapchain(){

}
void VKSwapchain::init(int w,int h){
    VkPhysicalDevice phys_device = device->get_phys_device();
    VkSurfaceKHR surface = device->get_surface();
    /** 1: capabilites */
    
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, surface, &capabilities);
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = capabilities.currentExtent;
    } else {
        extent.width = std::clamp(static_cast<uint32_t>(w), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(static_cast<uint32_t>(h), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    
    /** 2: formats */

    img_format = VK_FORMAT_B8G8R8A8_SRGB; 
    VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    /** 3: present modes */

    uint32_t imagec = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imagec > capabilities.maxImageCount) {
        imagec = capabilities.maxImageCount;
    }

    /** 4: init  */
    VkSwapchainCreateInfoKHR sw_create_info{};
    sw_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sw_create_info.surface = surface;
    sw_create_info.minImageCount = imagec;
    sw_create_info.imageFormat = img_format;
    sw_create_info.imageColorSpace = color_space;
    sw_create_info.imageExtent = extent;
    sw_create_info.imageArrayLayers = 1;
    sw_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 

    sw_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; 
    
    sw_create_info.preTransform = capabilities.currentTransform;
    sw_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sw_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sw_create_info.clipped = VK_TRUE;
    sw_create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device->get_logic_device(), &sw_create_info, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("swapchain init fail");
    }

    vkGetSwapchainImagesKHR(device->get_logic_device(), swapchain, &imagec, nullptr);
    img_v.resize(imagec);
    vkGetSwapchainImagesKHR(device->get_logic_device(), swapchain, &imagec, img_v.data());

    /** 5: image views */
    create_image_views();
}
void VKSwapchain::create_image_views() {
    imgview_v.resize(img_v.size());

    for (size_t i = 0; i < img_v.size(); i++) {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = img_v[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = img_format;
        
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device->get_logic_device(), &create_info, nullptr, &imgview_v[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views");
        }
    }
}
VkResult VKSwapchain::get_next_img(uint32_t* img_i, VkSemaphore pc_semaphore) {
    return vkAcquireNextImageKHR(
        device->get_logic_device(), 
        swapchain, 
        std::numeric_limits<uint64_t>::max(),
        pc_semaphore, 
        VK_NULL_HANDLE, 
        img_i
    );
}

VkResult VKSwapchain::present(uint32_t img_i, VkSemaphore rc_semaphore) {
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &rc_semaphore;

    VkSwapchainKHR swapchains[] = {swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &img_i;

    return vkQueuePresentKHR(device->get_presentq(), &present_info);
}
void VKSwapchain::recreate(ProgramState& ps) {
    for (auto imgv : imgview_v) {
        vkDestroyImageView(device->get_logic_device(),imgv, nullptr);
    }
    
    vkDestroySwapchainKHR(device->get_logic_device(), swapchain, nullptr);
    
    init(ps.w,ps.h);
}
