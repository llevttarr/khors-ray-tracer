#ifndef VK_PIPELINE_H
#define VK_PIPELINE_H

#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "app_util.h"
#include "vk_device.h"

enum class HitGroupType {
    Triangles, // VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR
    Procedural // VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR
};
class VKPipeline {
protected:
    std::shared_ptr<VKDevice> device;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipelineBindPoint bind_point;
    VKPipeline(std::shared_ptr<VKDevice> dev, VkPipeline p, VkPipelineLayout l, VkPipelineBindPoint bp)
        : device(dev), pipeline(p), layout(l), bind_point(bp) {}
public:
    virtual ~VKPipeline();

    VKPipeline(const VKPipeline&) = delete;
    VKPipeline& operator=(const VKPipeline&) = delete;
    VKPipeline(VKPipeline&&) = delete;
    VKPipeline& operator=(VKPipeline&&) = delete;

    void bind(VkCommandBuffer cmd) const;
    VkPipelineLayout get_layout() const { return layout; }
};
class VKPipelineBuilder {
protected:
    std::shared_ptr<VKDevice> device;
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    std::vector<VkPushConstantRange> push_constants;

    /** Layout creation */
    VkPipelineLayout build_pipeline_layout();

public:
    VKPipelineBuilder(std::shared_ptr<VKDevice> dev) : device(dev) {}
    virtual ~VKPipelineBuilder() = default;

    void add_descriptor_set_layout(VkDescriptorSetLayout layout);
    void add_push_constant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
};
#endif // VK_PIPELINE_H