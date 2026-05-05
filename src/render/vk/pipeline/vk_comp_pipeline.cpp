#include "vk_comp_pipeline.h"
VKComputePipelineBuilder&VKComputePipelineBuilder::set_shader(const std::string& spv_path) {
    shader_path = spv_path;
    return *this;
}
std::unique_ptr<VKComputePipeline> VKComputePipelineBuilder::build() {
    VkDevice d = device->get_logic_device();

    if (shader_path.empty())
        throw std::runtime_error("compute pipeline shader not set");

    VkShaderModule module = VKPipeline::load_shader(d,shader_path);
    VkPipelineLayout layout = build_pipeline_layout();

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = module;
    stage.pName = "main";
    VkComputePipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    info.stage = stage;
    info.layout = layout;

    VkPipeline pipeline;
    if (vkCreateComputePipelines(d, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(d, module, nullptr);
        throw std::runtime_error("failed to create compute pipeline");
    }

    vkDestroyShaderModule(d, module, nullptr);

    return std::unique_ptr<VKComputePipeline>(
        new VKComputePipeline(device, pipeline, layout)
    );
}