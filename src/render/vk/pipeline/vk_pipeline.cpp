#include "vk_pipeline.h"
/** Base */
VKPipeline::~VKPipeline() {
    VkDevice d = device->get_logic_device();

    if (pipeline) {
        vkDestroyPipeline(d, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }

    if (layout) {
        vkDestroyPipelineLayout(d, layout, nullptr);
        layout = VK_NULL_HANDLE;
    }
}
void VKPipeline::bind(VkCommandBuffer cmd) const {
    vkCmdBindPipeline(cmd, bind_point, pipeline);
}
VkShaderModule VKPipeline::load_shader(VkDevice device, const std::string& path) {
    auto code = read_filec(path);

    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(device, &ci, nullptr, &module) != VK_SUCCESS) {
        throw std::runtime_error("fail create shader module");
    }

    return module;
}

/**  Builder*/
VKPipelineBuilder& VKPipelineBuilder::add_descriptor_set_layout(VkDescriptorSetLayout layout) {
    descriptor_set_layouts.push_back(layout);
    return *this;
}
VKPipelineBuilder& VKPipelineBuilder::add_push_constant(VkShaderStageFlags stage_flags,uint32_t offset,uint32_t size) {
    VkPushConstantRange range{};
    range.stageFlags = stage_flags;
    range.offset = offset;
    range.size = size;

    push_constants.push_back(range);
    return *this;
}

VkPipelineLayout VKPipelineBuilder::build_pipeline_layout() {
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
    layout_info.pSetLayouts = descriptor_set_layouts.data();
    
    layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constants.size());
    layout_info.pPushConstantRanges = push_constants.data();

    VkPipelineLayout layout;
    if (vkCreatePipelineLayout(device->get_logic_device(), &layout_info, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("layout creation init fail");
    }
    return layout;
}