#ifndef VK_RT_PIPELINE_H
#define VK_RT_PIPELINE_H

#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "app_util.h"
#include "vk_device.h"
#include "vk_swapchain.h"

struct SBTRegions {
    VkStridedDeviceAddressRegionKHR raygen{};
    VkStridedDeviceAddressRegionKHR miss{};
    VkStridedDeviceAddressRegionKHR hit{};
    VkStridedDeviceAddressRegionKHR callable {};
};

class VKRTPipeline{
    friend class PipelineBuilder;
public:
    ~VKRTPipeline();

    void bind (VkCommandBuffer cmd) const;
    void trace(VkCommandBuffer cmd, uint32_t width, uint32_t height) const;

    VkPipeline get() const { return pipeline; }
    VkPipelineLayout get_layout() const { return layout; }
    const SBTRegions& get_sbt() const { return sbt; }

private:
    VKRTPipeline() = default;
    std::shared_ptr<VKDevice> device;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkBuffer sbt_buffer = VK_NULL_HANDLE;
    VmaAllocation sbt_alloc = VK_NULL_HANDLE;
    SBTRegions sbt{};
};

#endif //VK_RT_PIPELINE_H
