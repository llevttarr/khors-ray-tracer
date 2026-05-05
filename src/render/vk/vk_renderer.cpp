#include "vk_renderer.h"


VKRenderer::VKRenderer(std::shared_ptr<VKDevice> dev,std::shared_ptr<VKSwapchain>swap,std::unique_ptr<VKCmanager>cmgr,EulerCamera& cam)
    : device(std::move(dev)),swapchain(std::move(swap)),
cmanager(std::move(cmgr)),camera(cam),current_width (static_cast<uint32_t>(cam.get_w())),current_height(static_cast<uint32_t>(cam.get_h())), tri_buf  (device), sphr_buf(device), bvh_buf(device),
mat_buf  (device), prim_buf(device), light_buf(device), reservoir_b(device),
    cbuff_tex(device), accum_tex(device), refl_accum_tex(device),
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

    init_descriptor_layouts();
    init_camera_ubos();
    init_pipelines();

    create_restir_buffers();
    create_storage_images();
    update_pingpong_descriptors();
    update_output_descriptor();
    update_present_descriptor();
}

VKRenderer::~VKRenderer() {
    wait_idle();
    VkDevice vkdev = device->get_logic_device();
    for (auto dsl : { camera_dsl, scene_dsl, pingpong_dsl, output_dsl, present_dsl }) {
        if (dsl != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(vkdev, dsl, nullptr);
    }
}

void VKRenderer::init_pipelines() {
    constexpr VkShaderStageFlags PC_STAGES_COMPUTE =VK_SHADER_STAGE_COMPUTE_BIT;
    constexpr VkShaderStageFlags PC_STAGES_RT =VK_SHADER_STAGE_RAYGEN_BIT_KHR|VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR|VK_SHADER_STAGE_MISS_BIT_KHR;

    pipeline_res_sampling = VKRTPipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_push_constant(PC_STAGES_RT, 0, sizeof(ReSTIRPushConstants))
        .add_raygen_shader("assets/shaders/res_sampling.rgen.spv")
        .add_miss_shader("assets/shaders/res_sampling.rmiss.spv")
        .add_closest_hit_shader("assets/shaders/res_sampling.rchit.spv")
        .build();
    pipeline_temp_reuse = VKComputePipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_push_constant(PC_STAGES_COMPUTE, 0, sizeof(ReSTIRPushConstants))
        .set_shader("assets/shaders/temp_reuse.comp.spv")
        .build();
    pipeline_spat_reuse = VKComputePipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_push_constant(PC_STAGES_COMPUTE, 0, sizeof(ReSTIRPushConstants))
        .set_shader("assets/shaders/spat_reuse.comp.spv")
        .build();
    pipeline_res_shade = VKRTPipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_descriptor_set_layout(output_dsl)
        .add_push_constant(PC_STAGES_RT, 0, sizeof(ReSTIRPushConstants))
        .add_raygen_shader("assets/shaders/res_shade.rgen.spv")
        .add_miss_shader("assets/shaders/res_shade.rmiss.spv")
        .add_closest_hit_shader("assets/shaders/res_shade.rchit.spv")
        .build();
    pipeline_present = VKGraphicsPipelineBuilder(device)
        .add_descriptor_set_layout(present_dsl)
        .set_shaders("assets/shaders/present.vert.spv","assets/shaders/present.frag.spv")
        .set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .set_polygon_mode(VK_POLYGON_MODE_FILL)
        .set_cull_mode (VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disable_depth_test()
        .set_render_formats(swapchain->get_format(), VK_FORMAT_UNDEFINED)
        .build();
}
