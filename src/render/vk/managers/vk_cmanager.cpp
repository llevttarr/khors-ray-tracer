#include "vk_cmanager.h"

VKCmanager::VKCmanager(std::shared_ptr<VKDevice> dev, std::shared_ptr<VKSwapchain> swap):device(dev),swapchain(swap)
{

    /** 1: create the pool */
    
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = device->get_graphics_family();
    if (vkCreateCommandPool(device->get_logic_device(),&pool_info, nullptr, &command_pool) != VK_SUCCESS)
        throw std::runtime_error("init command pool fail");

    /** 2: alocate buffers */
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

    if (vkAllocateCommandBuffers(device->get_logic_device(),&alloc_info, command_buffers.data()) != VK_SUCCESS)
        throw std::runtime_error("command buffers allocation fail");

    /** 3: semaphores/fences */

    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo sem_info{};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(device->get_logic_device(),&sem_info, nullptr,&image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device->get_logic_device(),&sem_info, nullptr,&render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device->get_logic_device(),&fence_info, nullptr,&in_flight_fences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("parallel primitives init fail (command manager)");
        }
    }
}
VKCmanager::~VKCmanager(){
    VkDevice d = device->get_logic_device();
    vkDeviceWaitIdle(d);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(d, image_available_semaphores[i], nullptr);
        vkDestroySemaphore(d, render_finished_semaphores[i], nullptr);
        vkDestroyFence(d,in_flight_fences[i],nullptr);
    }
    vkDestroyCommandPool(d, command_pool, nullptr);
}
VkCommandBuffer VKCmanager::begin_frame(uint32_t& image_index){
    VkDevice d = device->get_logic_device();
    vkWaitForFences(d, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

    VkResult res= swapchain->get_next_img(&image_index,image_available_semaphores[current_frame]);

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        return VK_NULL_HANDLE;
    }
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to get swapchain image");

    vkResetFences(d, 1, &in_flight_fences[current_frame]);
    VkCommandBuffer cmd = command_buffers[current_frame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("failed to begin command buffer");

    return cmd;
}
void VKCmanager::begin_rendering(VkCommandBuffer cmd,VkImageView target_image_view,VkExtent2D extent,uint32_t image_index){
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.image = swapchain->get_image(image_index);
    barrier.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT,0, 1,0, 1
    };

    VkDependencyInfo dep_info{};
    dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd, &dep_info);

    VkRenderingAttachmentInfo colour_attachment{};
    colour_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colour_attachment.imageView = target_image_view;
    colour_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment.clearValue = {
        .color = {{0.0f, 0.0f, 0.0f, 1.0f}}
    };

    VkRenderingInfo rendering_info{};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea = { {0, 0}, extent };
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &colour_attachment;

    vkCmdBeginRendering(cmd, &rendering_info);
}
void VKCmanager::end_rendering(VkCommandBuffer cmd,uint32_t image_index) {
    vkCmdEndRendering(cmd);

    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_NONE;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.image = swapchain->get_image(image_index);
    barrier.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VkDependencyInfo dep_info{};
    dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmd, &dep_info);
}
void VKCmanager::end_frame_and_submit(VkCommandBuffer cmd,uint32_t image_index){
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
        throw std::runtime_error("VKCmanager: failed to end command buffer");

    VkSemaphoreSubmitInfo wait_info{};
    wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    wait_info.semaphore = image_available_semaphores[current_frame];
    wait_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    wait_info.value = 0; 

    VkSemaphoreSubmitInfo signal_info{};
    signal_info.sType= VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signal_info.semaphore = render_finished_semaphores[current_frame];
    signal_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    signal_info.value = 0;

    VkCommandBufferSubmitInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmd_info.commandBuffer = cmd;

    VkSubmitInfo2 submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.waitSemaphoreInfoCount = 1;
    submit_info.pWaitSemaphoreInfos = &wait_info;
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &cmd_info;
    submit_info.signalSemaphoreInfoCount = 1;
    submit_info.pSignalSemaphoreInfos = &signal_info;

    if (vkQueueSubmit2(device->get_graphicsq(), 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS)
        throw std::runtime_error("q submit fail");

    VkResult result = swapchain->present(image_index,render_finished_semaphores[current_frame]);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // TODO
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("present failed");
    }

    current_frame = (current_frame+1) % MAX_FRAMES_IN_FLIGHT;
}
