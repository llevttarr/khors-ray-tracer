#include "vk_renderer.h"

void VKRenderer::create_restir_buffers() {
    const VkDeviceSize res_size = static_cast<VkDeviceSize>(current_width)* current_height * sizeof(Reservoir);
    const VkDeviceSize gbuf_size = static_cast<VkDeviceSize>(current_width)* current_height * sizeof(GBufferPixel);
 
    constexpr VkBufferUsageFlags BUF_FLAGS =VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT;
 
    for (int i = 0; i < 2; ++i) {
        reservoir_ab[i].destroy();
        reservoir_ab[i].create(res_size,BUF_FLAGS, VMA_MEMORY_USAGE_GPU_ONLY);
        gbuffer_ab[i].destroy();
        gbuffer_ab[i].create(gbuf_size, BUF_FLAGS, VMA_MEMORY_USAGE_GPU_ONLY);
    }
    reservoir_b.destroy();
    reservoir_b.create(res_size, BUF_FLAGS, VMA_MEMORY_USAGE_GPU_ONLY);
}
void VKRenderer::update_scene(RenderScene& scene) {
    scene_util::build_emissive(scene);
    wait_idle();
    tric = static_cast<uint32_t>(scene.tri_v.size());
    std::cout<<"Triangle count: "<<tric<<std::endl;
    spherec = static_cast<uint32_t>(scene.sphr_v.size());
    bvhc = static_cast<uint32_t>(scene.bvh_v.size());
    // std::cout<<"bvhc: "<<bvhc<<std::endl;
    matc = static_cast<uint32_t>(scene.mat_v.size());
    std::cout<<"Material count: "<<matc<<std::endl;
    lightc = static_cast<uint32_t>(scene.light_v.size());
    std::cout<<"Light count: "<<matc<<std::endl;

    upload_scene_buffers(scene);
    if (tric!=0){
        blas = accel_builder->build_blas(scene.tri_v);
        tlas = accel_builder->build_tlas(*blas);
    }else{
        blas.reset();
        tlas = accel_builder->build_empty_tlas();
    }

    create_texture_arrays(scene);
    update_scene_descriptor();
}
void VKRenderer::update_mats(RenderScene& scene){
    // FIXME
    update_scene(scene);
}
void VKRenderer::update_lights(RenderScene& scene){
    // FIXME
    update_scene(scene);
}
void VKRenderer::upload_scene_buffers(RenderScene& scene) {
    auto upload = [&](VKBuffer& dst, const void* data, VkDeviceSize size) {
        const VkDeviceSize safe_size = (size == 0) ? 16 : size;

        dst.destroy();
        dst.create(
            safe_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
        if (size == 0) {
            return;
        }
        VKBuffer staging(device);
        staging.create(
            size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY,
            VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
        staging.write(data, size);
        one_time_submit([&](VkCommandBuffer cmd) {
            VkBufferCopy region{ 0, 0, size };
            vkCmdCopyBuffer(cmd, staging.get(), dst.get(), 1, &region);

            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = dst.get();
            barrier.offset = 0;
            barrier.size = safe_size;

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0,
                0, nullptr,
                1, &barrier,
                0, nullptr
            );
        });
    };
 
    upload(tri_buf,scene.tri_v.data(),scene.tri_v.size() * sizeof(RenderTri));
    upload(sphr_buf,scene.sphr_v.data(), scene.sphr_v.size() * sizeof(Sphr));
    upload(bvh_buf,scene.bvh_v.data(), scene.bvh_v.size() * sizeof(BVH));
    upload(mat_buf,scene.mat_v.data(),scene.mat_v.size()* sizeof(Mat));
    upload(prim_buf,scene.prim_v.data(), scene.prim_v.size() * sizeof(uint32_t));
    upload(light_buf,scene.light_v.data(),scene.light_v.size()* sizeof(Light));
}
 