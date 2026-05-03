#ifndef VK_CMANAGER_H
#define VK_CMANAGER_H

#include <vector>
#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "vk_device.h"
#include "vk_swapchain.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

/**
 * VkCommandBuffer wrapper, handles sync
 */
class VKCmanager{
private:
    std::shared_ptr<VKDevice> device;
    std::shared_ptr<VKSwapchain> swapchain;
    uint32_t current_frame = 0;

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
public:
    VKCmanager(std::shared_ptr<VKDevice> dev, std::shared_ptr<VKSwapchain> swap);
    ~VKCmanager();

    VkCommandBuffer begin_frame(uint32_t& image_index);
    void begin_rendering(VkCommandBuffer cmd, VkImageView target_image_view, VkExtent2D extent);
    void end_rendering(VkCommandBuffer cmd);
    void end_frame_and_submit(VkCommandBuffer cmd, uint32_t image_index);
};

#endif // VK_CMANAGER_H