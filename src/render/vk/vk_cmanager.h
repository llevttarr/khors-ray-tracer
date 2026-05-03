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
    VKCmanager(VKDevice* dev, VKSwapchain* swap);
    ~VKCmanager();
    /**
     * Get next swapchain image, then begin adding to the command buffer
     */
    VkCommandBuffer begin_frame(uint32_t& image_index);
    /**
     * Transitions the swapchain image to COLOR_ATTACHMENT_OPTIMAL, then opens a dynamic render pass
     */
    void begin_rendering(VkCommandBuffer cmd, VkImageView target_image_view, VkExtent2D extent,uint32_t image_index);
    /**
     * Closes the dynamic render pass; swapchain can display the image
     */
    void end_rendering(VkCommandBuffer cmd,uint32_t image_index);

    /**
     * ends and submits via Synchronization2
     */
    void end_frame_and_submit(VkCommandBuffer cmd, uint32_t image_index);
};

#endif // VK_CMANAGER_H