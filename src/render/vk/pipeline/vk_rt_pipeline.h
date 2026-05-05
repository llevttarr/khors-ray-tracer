#ifndef VK_RT_PIPELINE_H
#define VK_RT_PIPELINE_H

#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "app_util.h"
#include "vk_device.h"
#include "vk_pipeline.h"
#include "glsl_util.h"

struct SBTRegionsRT {
    VkStridedDeviceAddressRegionKHR raygen{};
    VkStridedDeviceAddressRegionKHR miss{};
    VkStridedDeviceAddressRegionKHR hit{};
    VkStridedDeviceAddressRegionKHR callable {};
};

class VKRTPipeline : public VKPipeline {
    friend class VKRTPipelineBuilder;
private:
    VkBuffer sbt_buffer = VK_NULL_HANDLE;
    VmaAllocation sbt_alloc = VK_NULL_HANDLE;
    SBTRegionsRT sbt_regions{};
    VKRTPipeline(std::shared_ptr<VKDevice> dev, VkPipeline p, VkPipelineLayout l)
        : VKPipeline(dev, p, l, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR) {}

    // void trace(VkCommandBuffer cmd, const SBTRegionsRT& regions,uint32_t width, uint32_t height, uint32_t depth = 1) const; - CManager will do that prolly
public:
    const SBTRegionsRT& get_sbt() const { return sbt_regions; }
};
class VKRTPipelineBuilder: public VKPipelineBuilder{
private:
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups;
    uint32_t max_recursion_depth = 1;
    uint32_t n_raygen = 1;
    uint32_t n_miss = 0;
    uint32_t n_hit = 0;

public:
    VKRTPipelineBuilder(std::shared_ptr<VKDevice> dev) : VKPipelineBuilder(dev) {}
    VKRTPipelineBuilder& add_raygen_shader(const std::string& spv_path);
    VKRTPipelineBuilder& add_miss_shader(const std::string& spv_path);
    VKRTPipelineBuilder& add_closest_hit_shader(const std::string& spv_path);
    VKRTPipelineBuilder& set_max_recursion_depth(uint32_t depth);

    std::unique_ptr<VKRTPipeline> build();
};

#endif //VK_RT_PIPELINE_H
