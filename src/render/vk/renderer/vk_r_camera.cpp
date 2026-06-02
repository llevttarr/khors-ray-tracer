#include "vk_renderer.h"

void VKRenderer::init_camera_ubos() {
    VkDevice vkdev = device->get_logic_device();
 
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        camera_ubos[i].create(
            sizeof(CameraUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
        descriptor_allocator->allocate(&camera_sets[i], camera_dsl);
 
        VkDescriptorBufferInfo buf_info{};
        buf_info.buffer = camera_ubos[i].get();
        buf_info.offset = 0;
        buf_info.range = sizeof(CameraUBO);
 
        VkWriteDescriptorSet w{};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = camera_sets[i];
        w.dstBinding = 0;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w.pBufferInfo = &buf_info;
        vkUpdateDescriptorSets(vkdev, 1, &w, 0, nullptr);
    }
}
void VKRenderer::update_camera_ubo(uint32_t frame_idx) {
    const float aspect = static_cast<float>(current_width) /static_cast<float>(current_height);
 
    CameraUBO ubo{};
    ubo.pos = Vec4<float>(camera.get_pos().x,camera.get_pos().y,camera.get_pos().z,camera.get_fov());
    ubo.forward = Vec4<float>(camera.get_forward().x,camera.get_forward().y,camera.get_forward().z,aspect);
    ubo.right = Vec4<float>(camera.get_right().x,camera.get_right().y,camera.get_right().z,0.0f);
    ubo.up =Vec4<float>(camera.get_up().x,camera.get_up().y,camera.get_up().z,0.0f);
 
    if (prev_camera_valid) {
        ubo.prev_pos = Vec4<float>(prev_camera.pos.x,prev_camera.pos.y,prev_camera.pos.z,prev_camera.fov);
        ubo.prev_forward = Vec4<float>(prev_camera.forward.x,prev_camera.forward.y,prev_camera.forward.z,prev_camera.aspect);
        ubo.prev_right = Vec4<float>(prev_camera.right.x,prev_camera.right.y,prev_camera.right.z,0.0f);
        ubo.prev_up = Vec4<float>(prev_camera.up.x,prev_camera.up.y,prev_camera.up.z,0.0f);
    } else {
        ubo.prev_pos = ubo.pos;
        ubo.prev_forward = ubo.forward;
        ubo.prev_right = ubo.right;
        ubo.prev_up= ubo.up;
    }
    camera_ubos[frame_idx].write(&ubo, sizeof(CameraUBO));
}
bool VKRenderer::camera_moved() const {
    if (!prev_camera_valid){
        return false;
    }
    return camera.get_pos()!= prev_camera.pos|| camera.get_forward() != prev_camera.forward || camera.get_fov() != prev_camera.fov;
}