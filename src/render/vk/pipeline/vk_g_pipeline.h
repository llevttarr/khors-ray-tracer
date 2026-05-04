#ifndef VK_G_PIPELINE_H
#define VK_G_PIPELINE_H

#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "app_util.h"
#include "vk_device.h"
#include "vk_pipeline.h"

class VKGraphicsPipeline : public VKPipeline {
friend class VKGraphicsPipelineBuilder;
private:
    VKGraphicsPipeline(std::shared_ptr<VKDevice> dev, VkPipeline p, VkPipelineLayout l)
        : VKPipeline(dev, p, l, VK_PIPELINE_BIND_POINT_GRAPHICS) {}
};
class VKGraphicsPipelineBuilder : public VKPipelineBuilder{
private:
    std::shared_ptr<VKDevice> device;
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    
    VkFormat color_format = VK_FORMAT_UNDEFINED;
    VkFormat depth_format = VK_FORMAT_UNDEFINED;

    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState color_blend_attachment;
    VkPipelineDepthStencilStateCreateInfo depth_stencil;

public:
    VKGraphicsPipelineBuilder(std::shared_ptr<VKDevice> dev): VKPipelineBuilder(dev) {}

    VKGraphicsPipelineBuilder& set_shaders(const std::string& vs_path, const std::string& fs_path);
    VKGraphicsPipelineBuilder& set_topology(VkPrimitiveTopology topology);
    VKGraphicsPipelineBuilder& set_polygon_mode(VkPolygonMode mode);
    VKGraphicsPipelineBuilder& set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    VKGraphicsPipelineBuilder& disable_depth_test();
    VKGraphicsPipelineBuilder& enable_depth_test(VkCompareOp compareOp);
    
    VKGraphicsPipelineBuilder& set_render_formats(VkFormat color, VkFormat depth);

    std::unique_ptr<VKGraphicsPipeline> build();
};
#endif // VK_G_PIPELINE_H