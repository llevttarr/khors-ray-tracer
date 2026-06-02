#include "vk_renderer.h"
PushConstants VKRenderer::make_push_constants() const {
    return PushConstants{ current_width,current_height,tric,spherec,bvhc,matc,lightc,framec,init_candidates_restir, };
}
void VKRenderer::buf_barrier(VkCommandBuffer cmd,VkBuffer buf,VkDeviceSize size,VkPipelineStageFlags2 src_stage,VkAccessFlags2 src_access,VkPipelineStageFlags2 dst_stage,VkAccessFlags2 dst_access){
    VkBufferMemoryBarrier2 b{};
    b.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    b.srcStageMask = src_stage;
    b.srcAccessMask = src_access;
    b.dstStageMask = dst_stage;
    b.dstAccessMask = dst_access;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.buffer = buf;
    b.offset= 0;
    b.size = size;
 
    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = 1;
    dep.pBufferMemoryBarriers = &b;
    vkCmdPipelineBarrier2(cmd, &dep);

}
void VKRenderer::img_barrier(VkCommandBuffer cmd,VkImage img,VkPipelineStageFlags2 src_stage,VkAccessFlags2 src_access,VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access,VkImageLayout old_layout,VkImageLayout new_layout, uint32_t base_layer, uint32_t layer_count){
    VkImageMemoryBarrier2 b{};
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    b.srcStageMask = src_stage;
    b.srcAccessMask = src_access;
    b.dstStageMask = dst_stage;
    b.dstAccessMask = dst_access;
    b.oldLayout = old_layout;
    b.newLayout = new_layout;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img;
    b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, base_layer, layer_count };
 
    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &b;
    vkCmdPipelineBarrier2(cmd, &dep);

}
void VKRenderer::wait_idle(){
    vkDeviceWaitIdle(device->get_logic_device());
}
