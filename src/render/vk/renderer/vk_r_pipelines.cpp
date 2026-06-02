#include "vk_renderer.h"

void VKRenderer::init_pipelines() {
    std::cout<<"init pipeliens"<<std::endl;
    constexpr VkShaderStageFlags PC_STAGES_COMPUTE =VK_SHADER_STAGE_COMPUTE_BIT;
    constexpr VkShaderStageFlags PC_STAGES_RT =VK_SHADER_STAGE_RAYGEN_BIT_KHR|VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR|VK_SHADER_STAGE_MISS_BIT_KHR;

    pipeline_res_sampling = VKRTPipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_push_constant(PC_STAGES_RT, 0, sizeof(PushConstants))
        .add_raygen_shader("assets/shaders/vk_res_sampling.rgen.spv")
        .add_miss_shader("assets/shaders/vk_res_sampling.rmiss.spv")
        .add_closest_hit_shader("assets/shaders/vk_res_sampling.rchit.spv")
        .build();
    pipeline_temp_reuse = VKComputePipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_push_constant(PC_STAGES_COMPUTE, 0, sizeof(PushConstants))
        .set_shader("assets/shaders/vk_temp_reuse.comp.spv")
        .build();
    pipeline_spat_reuse = VKComputePipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_push_constant(PC_STAGES_COMPUTE, 0, sizeof(PushConstants))
        .set_shader("assets/shaders/vk_spat_reuse.comp.spv")
        .build();
    pipeline_res_shade = VKRTPipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_descriptor_set_layout(output_dsl)
        .add_push_constant(PC_STAGES_RT, 0, sizeof(PushConstants))
        .add_raygen_shader("assets/shaders/vk_res_shade.rgen.spv")
        .add_miss_shader("assets/shaders/vk_res_shade_refl.rmiss.spv")
        .add_miss_shader("assets/shaders/vk_res_shade_shadow.rmiss.spv")
        .add_closest_hit_shader("assets/shaders/vk_res_shade.rchit.spv")
        .set_max_recursion_depth(2)
        .build();
    pipeline_present = VKGraphicsPipelineBuilder(device)
        .add_descriptor_set_layout(present_dsl)
        .set_shaders("assets/shaders/vk_vs.vert.spv","assets/shaders/vk_fs.frag.spv")
        .set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .set_polygon_mode(VK_POLYGON_MODE_FILL)
        .set_cull_mode (VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disable_depth_test()
        .set_render_formats(swapchain->get_format(), VK_FORMAT_UNDEFINED)
        .build();
}

void VKRenderer::record_present_pass(VkCommandBuffer cmd) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(current_width);
    viewport.height = static_cast<float>(current_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { current_width, current_height };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    pipeline_present->bind(cmd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_present->get_layout(), 0, 1, &present_set, 0, nullptr);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}
void VKRenderer::dispatch_res_sampling(VkCommandBuffer cmd,uint32_t dx, uint32_t dy) {
    pipeline_res_sampling->bind(cmd);
 
    const VkPipelineLayout layout = pipeline_res_sampling->get_layout();
    const std::array<VkDescriptorSet, 3> sets = {
        camera_sets[cmanager->get_current_frame()],
        scene_set,
        pingpong_sets[pingpong_index],
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
 
    const auto pc = make_push_constants();
    vkCmdPushConstants(cmd, layout,VK_SHADER_STAGE_RAYGEN_BIT_KHR |VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR|VK_SHADER_STAGE_MISS_BIT_KHR,0, sizeof(pc), &pc);
 
    const auto& sbt = pipeline_res_sampling->get_sbt();
    vkCmdTraceRaysKHR(cmd,&sbt.raygen, &sbt.hit, &sbt.miss, &sbt.callable,current_width, current_height, 1);
}
 
void VKRenderer::dispatch_temp_reuse(VkCommandBuffer cmd,uint32_t dx, uint32_t dy) {
    pipeline_temp_reuse->bind(cmd);
    const VkPipelineLayout layout = pipeline_temp_reuse->get_layout();
    const std::array<VkDescriptorSet, 3> sets = {
        camera_sets[cmanager->get_current_frame()],
        scene_set,
        pingpong_sets[pingpong_index],
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    const auto pc = make_push_constants();
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(pc), &pc);
    vkCmdDispatch(cmd, dx, dy, 1);
}
 
void VKRenderer::dispatch_spat_reuse(VkCommandBuffer cmd,uint32_t dx, uint32_t dy) {
    pipeline_spat_reuse->bind(cmd);
    const VkPipelineLayout layout = pipeline_spat_reuse->get_layout();
    const std::array<VkDescriptorSet, 3> sets = {
        camera_sets[cmanager->get_current_frame()],
        scene_set,
        pingpong_sets[pingpong_index],
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    const auto pc = make_push_constants();
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(pc), &pc);
    vkCmdDispatch(cmd, dx, dy, 1);
}
 
void VKRenderer::dispatch_res_shade(VkCommandBuffer cmd,uint32_t dx, uint32_t dy) {
    pipeline_res_shade->bind(cmd);
    const VkPipelineLayout layout = pipeline_res_shade->get_layout();
    const std::array<VkDescriptorSet, 4> sets = {
        camera_sets[cmanager->get_current_frame()],
        scene_set,
        pingpong_sets[pingpong_index],
        output_set,   // cbuf; accum;refl_accum
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    const auto pc = make_push_constants();
    vkCmdPushConstants(cmd, layout,VK_SHADER_STAGE_RAYGEN_BIT_KHR |VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR|VK_SHADER_STAGE_MISS_BIT_KHR,0, sizeof(pc), &pc);
 
    const auto& sbt = pipeline_res_shade->get_sbt();
    vkCmdTraceRaysKHR(cmd,&sbt.raygen, &sbt.miss, &sbt.hit, &sbt.callable,current_width, current_height, 1);
}