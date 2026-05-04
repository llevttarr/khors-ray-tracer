#include "vk_pipeline.h"
/** Base */

/**  Builder*/
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