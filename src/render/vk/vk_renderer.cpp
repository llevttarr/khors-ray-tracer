#include "vk_renderer.h"


VKRenderer::VKRenderer(std::shared_ptr<VKDevice> dev,std::shared_ptr<VKSwapchain>swap,std::unique_ptr<VKCmanager>cmgr,EulerCamera& cam)
    : device(std::move(dev)),swapchain(std::move(swap)),
cmanager(std::move(cmgr)),camera(cam),current_width (static_cast<uint32_t>(cam.get_w())),current_height(static_cast<uint32_t>(cam.get_h())), tri_buf  (device), sphr_buf(device), bvh_buf(device),
mat_buf  (device), prim_buf(device), light_buf(device), reservoir_b(device),
    cbuff_tex(device), accum_tex(device), refl_accum_tex(device), bloom_tex_a(device),bloom_tex_b(device),
    base_tex_arr(device), normal_tex_arr(device), specular_tex_arr(device)
{
    for (int i = 0; i < 2; ++i) {
        reservoir_ab[i] = VKBuffer(device);
        gbuffer_ab[i] = VKBuffer(device);
    }
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        camera_ubos[i] = VKBuffer(device);
    }

    descriptor_allocator = std::make_unique<VKDescriptorAllocator>(device);

    accel_builder = std::make_unique<VKAccelBuilder>(device,[this](const std::function<void(VkCommandBuffer)>& fn){ one_time_submit(fn); });

    init_descriptor_layouts();
    init_camera_ubos();
    init_pipelines();

    create_restir_buffers();
    create_storage_images();
    update_pingpong_descriptors();
    update_output_descriptor();
    update_present_descriptor();
    update_refl_output_descriptor();
    update_postp_descriptor();
}

VKRenderer::~VKRenderer() {
    wait_idle();
    VkDevice vkdev = device->get_logic_device();
    for (auto dsl : { camera_dsl, scene_dsl, pingpong_dsl, output_dsl, present_dsl }) {
        if (dsl != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(vkdev, dsl, nullptr);
    }
}

// void VKRenderer::run_rs_test() {
//     if (framebuffer_resized || static_cast<uint32_t>(camera.get_w()) != current_width || static_cast<uint32_t>(camera.get_h()) != current_height) {
//         on_window_resize(camera.get_w(), camera.get_h());
//     }
//     if (camera_moved()) {
//         framec = 0;
//     }
//     std::cout << "run_rs" << std::endl;
 
//     uint32_t image_index = 0;
//     VkCommandBuffer cmd  = cmanager->begin_frame(image_index);
//     if (cmd == VK_NULL_HANDLE) return;
 
//     const uint32_t frame_idx = cmanager->get_current_frame();
//     update_camera_ubo(frame_idx);

//     img_barrier(cmd, cbuff_tex.get_image(),
//         VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0, 
//         VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
//         VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

//     const VkExtent2D ext = swapchain->get_extent();
    
//     cmanager->begin_rendering(cmd, swapchain->get_image_view(image_index), ext, image_index);
    
//     record_present_pass(cmd); 
    
//     cmanager->end_rendering(cmd, image_index);
//     img_barrier(cmd, cbuff_tex.get_image(),
//         VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
//         VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
//         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

//     cmanager->end_frame_and_submit(cmd, image_index);
 
//     framec++;
//     prev_camera = {
//         camera.get_pos(),
//         camera.get_forward(),
//         camera.get_right(),
//         camera.get_up(),
//         camera.get_fov(),
//         static_cast<float>(current_width) / static_cast<float>(current_height)
//     };
//     prev_camera_valid = true;
//     pingpong_index ^= 1;
// }

void VKRenderer::run_rs(std::function<void(VkCommandBuffer)> ui_draw_fn) {
    if (framebuffer_resized ||static_cast<uint32_t>(camera.get_w()) != current_width ||static_cast<uint32_t>(camera.get_h()) != current_height) {
        on_window_resize(camera.get_w(), camera.get_h());
    }
    if (camera_moved()) {
        framec = 0;
    }
 
    uint32_t image_index = 0;
    VkCommandBuffer cmd  = cmanager->begin_frame(image_index);
    if (cmd == VK_NULL_HANDLE) return;
 
    const uint32_t frame_idx = cmanager->get_current_frame();
 
    update_camera_ubo(frame_idx);
 
    const uint32_t dx = (current_width  + LOCAL_SIZE_X - 1) / LOCAL_SIZE_X;
    const uint32_t dy = (current_height + LOCAL_SIZE_Y - 1) / LOCAL_SIZE_Y;
 
    dispatch_res_sampling(cmd, dx, dy);
 
    buf_barrier(cmd, reservoir_ab[pingpong_index].get(), VK_WHOLE_SIZE,
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
    buf_barrier(cmd, gbuffer_ab[pingpong_index].get(), VK_WHOLE_SIZE,
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT| VK_ACCESS_2_TRANSFER_READ_BIT);
 
    if (framec == 0) {
        VkBufferCopy region{};
        region.size = static_cast<VkDeviceSize>(current_width)
                    * current_height * sizeof(GBufferPixel);
        vkCmdCopyBuffer(cmd,
            gbuffer_ab[pingpong_index].get(),
            gbuffer_ab[1 - pingpong_index].get(),
            1, &region);
        buf_barrier(cmd, gbuffer_ab[1 - pingpong_index].get(), VK_WHOLE_SIZE,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
    }
 
    dispatch_temp_reuse(cmd, dx, dy);
    buf_barrier(cmd, reservoir_b.get(), VK_WHOLE_SIZE,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);

    dispatch_spat_reuse(cmd, dx, dy);
    buf_barrier(cmd, reservoir_ab[pingpong_index].get(), VK_WHOLE_SIZE,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_READ_BIT);

    dispatch_res_shade(cmd, dx, dy);
    img_barrier(cmd, cbuff_tex.get_image(),
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    
    dispatch_refl_trace(cmd,dx,dy);
    img_barrier(cmd, refl_accum_tex.get_image(),
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    dispatch_accumulation(cmd,dx,dy);
    img_barrier(cmd, cbuff_tex.get_image(),
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    img_barrier(cmd, accum_tex.get_image(),
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    dispatch_fog(cmd,dx,dy);
    img_barrier(cmd, cbuff_tex.get_image(),
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
    dispatch_bloom(cmd,dx,dy);
    img_barrier(cmd, cbuff_tex.get_image(),
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    const VkExtent2D ext = swapchain->get_extent();
    cmanager->begin_rendering(cmd, swapchain->get_image_view(image_index), ext, image_index);
    record_present_pass(cmd);
    if(ui_draw_fn){
        ui_draw_fn(cmd);
    }
    cmanager->end_rendering(cmd, image_index);
    img_barrier(cmd, cbuff_tex.get_image(),
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    cmanager->end_frame_and_submit(cmd, image_index);
 
    framec++;
    prev_camera = {
        camera.get_pos(),
        camera.get_forward(),
        camera.get_right(),
        camera.get_up(),
        camera.get_fov(),
        static_cast<float>(current_width) / static_cast<float>(current_height)
    };
    prev_camera_valid = true;
    pingpong_index ^= 1;
}

void VKRenderer::on_window_resize(uint32_t w, uint32_t h){
    wait_idle();
 
    current_width = w;
    current_height = h;
    framebuffer_resized = false;
    framec = 0;
 
    create_restir_buffers();
    create_storage_images();
 
    descriptor_allocator->reset_pools();
 
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        descriptor_allocator->allocate(&camera_sets[i], camera_dsl);
 
        VkDescriptorBufferInfo bi{ camera_ubos[i].get(), 0, sizeof(CameraUBO) };
        VkWriteDescriptorSet w{};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet= camera_sets[i];
        w.dstBinding = 0;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w.pBufferInfo= &bi;
        vkUpdateDescriptorSets(device->get_logic_device(), 1, &w, 0, nullptr);
    }

    update_scene_descriptor();
    update_pingpong_descriptors();
    update_output_descriptor();
    update_present_descriptor();
    update_refl_output_descriptor();
    update_postp_descriptor();
}
