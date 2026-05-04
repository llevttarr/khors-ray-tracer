#ifndef VK_PIPELINE_BUILDER_H
#define VK_PIPELINE_BUILDER_H

#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "app_util.h"
#include "vk_device.h"
#include "vk_rt_pipeline.h"
#include "vk_swapchain.h"

enum class HitGroupType {
    Triangles, // VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR
    Procedural // VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR
};

/**
 * the actual builder for rt pipeline
 */
class PipelineBuilder{
private:
    struct OwnedStage {
        VkPipelineShaderStageCreateInfo ci{};
        VkShaderModule module = VK_NULL_HANDLE;
    };
    VkShaderModule load_spv(const std::string& path);
    void build_sbt(VKRTPipeline& p, uint32_t group_count);

    std::vector<OwnedStage> stages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    std::vector<VkPushConstantRange> push_constant_ranges;

    uint32_t max_recursion_depth = 1;
    VkPipelineCache pipeline_cache = VK_NULL_HANDLE;

    uint32_t n_raygen = 0;
    uint32_t n_miss = 0;
    uint32_t n_hit = 0;
public:
    explicit PipelineBuilder(std::shared_ptr<VKDevice> dev);

    PipelineBuilder& add_raygen (const std::string& spv_path);
    PipelineBuilder& add_miss (const std::string& spv_path);
    PipelineBuilder& add_hit_group(const std::string& closest_hit_spv,
                                   HitGroupType type = HitGroupType::Triangles,
                                   const std::string& any_hit_spv = "",const std::string& intersect_spv = "");
    PipelineBuilder& add_descriptor_set_layout(VkDescriptorSetLayout dsl);

    template<typename T>
    PipelineBuilder& add_push_constant(VkShaderStageFlags stages, uint32_t offs = 0){
        VkPushConstantRange r{};
        r.stageFlags = stages;
        r.offset = offs;
        r.size = sizeof(T);
        push_constant_ranges.push_back(r);
        return *this;
    }
    PipelineBuilder& set_max_recursion_depth(uint32_t depth);
    PipelineBuilder& set_pipeline_cache(VkPipelineCache cache);
    std::unique_ptr<VKRTPipeline> build();

};
#endif // VK_PIPELINE_H