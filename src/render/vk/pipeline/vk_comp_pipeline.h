#ifndef VK_COMP_PIPELINE_H
#define VK_COMP_PIPELINE_H

#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "app_util.h"
#include "vk_device.h"
#include "vk_pipeline.h"

class VKComputePipeline : public VKPipeline{
    friend class VKComputePipelineBuilder;
private:
    VKComputePipeline(std::shared_ptr<VKDevice> dev, VkPipeline p, VkPipelineLayout l)
        : VKPipeline(dev, p, l, VK_PIPELINE_BIND_POINT_COMPUTE) {}
    // void dispatch(VkCommandBuffer cmd, uint32_t gx, uint32_t gy, uint32_t gz = 1) const; - CManager ??
};
class VKComputePipelineBuilder : public VKPipelineBuilder{
private:
    std::string shader_path;
public:
    VKComputePipelineBuilder(std::shared_ptr<VKDevice> dev) : VKPipelineBuilder(dev) {}
    VKComputePipelineBuilder& set_shader(const std::string& spv_path);
    std::unique_ptr<VKComputePipeline> build();

    VKComputePipelineBuilder& add_descriptor_set_layout(VkDescriptorSetLayout layout) override {
        VKPipelineBuilder::add_descriptor_set_layout(layout);
        return *this;
    };
    VKComputePipelineBuilder& add_push_constant(VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size) override {
        VKPipelineBuilder::add_push_constant(stage_flags,offset,size);
        return *this;
    };
};
#endif //VK_COMP_PIPELINE_H
