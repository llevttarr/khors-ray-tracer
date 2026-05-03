#ifndef VK_SWAPCHAIN_H
#define VK_SWAPCHAIN_H

#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "vk_device.h"
/**
 * Manages images to the screen
 */
class VKSwapchain{
private:
    std::shared_ptr<VKDevice> device;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> img_v;
    std::vector<VkImageView> imgview_v;
    VkFormat img_format;
    VkExtent2D extent;
public:
    VKSwapchain();
    ~VKSwapchain();

    void recreate(int w, int h);
    VkResult present(uint32_t img_i, VkSemaphore rc_semaphore);
    VkResult get_next_img(uint32_t* img_i, VkSemaphore pc_semaphore);
};

#endif // VK_SWAPCHAIN_H