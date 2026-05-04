#ifndef VK_COMP_PIPELINE_H
#define VK_COMP_PIPELINE_H

#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "app_util.h"
#include "vk_device.h"
#include "vk_swapchain.h"

class VKComputePipeline{
private:
    std::shared_ptr<VKDevice> device;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout   = VK_NULL_HANDLE;
public:
    VKComputePipeline(std::shared_ptr<VKDevice> dev, const std::string& spv_path,
                      std::vector<VkDescriptorSetLayout> layouts = {},std::vector<VkPushConstantRange> push_ranges = {});
    ~VKComputePipeline();

    VKComputePipeline(const VKComputePipeline&) = delete;
    VKComputePipeline& operator=(const VKComputePipeline&) = delete;
    VKComputePipeline(VKComputePipeline&&) = delete;
    VKComputePipeline& operator=(VKComputePipeline&&) = delete;

    void bind(VkCommandBuffer cmd) const;
    void dispatch(VkCommandBuffer cmd, uint32_t gx, uint32_t gy, uint32_t gz = 1) const;

    VkPipeline get() const {return pipeline; }
    VkPipelineLayout get_layout() const { return layout; }

};
#endif //VK_COMP_PIPELINE_H
