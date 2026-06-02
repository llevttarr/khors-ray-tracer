#include "vk_renderer.h"
void VKRenderer::one_time_submit(const std::function<void(VkCommandBuffer)>& fn){
    VkCommandPoolCreateInfo pool_ci{};
    pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    pool_ci.queueFamilyIndex = device->get_graphics_family();
 
    VkCommandPool tmp_pool = VK_NULL_HANDLE;
    vkCreateCommandPool(device->get_logic_device(), &pool_ci, nullptr, &tmp_pool);
 
    VkCommandBufferAllocateInfo alloc_ci{};
    alloc_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_ci.commandPool = tmp_pool;
    alloc_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_ci.commandBufferCount = 1;
 
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device->get_logic_device(), &alloc_ci, &cmd);
 
    VkCommandBufferBeginInfo begin_ci{};
    begin_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_ci.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin_ci);
 
    fn(cmd);

    vkEndCommandBuffer(cmd); 
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(device->get_graphicsq(), 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(device->get_graphicsq());
    vkDestroyCommandPool(device->get_logic_device(), tmp_pool, nullptr);
}