#ifndef VK_GPU_TIMER_H
#define VK_GPU_TIMER_H

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>
class VkGpuTimerScope {
private:
    VkCommandBuffer cmd;
    VkQueryPool pool;
public:
    VkGpuTimerScope(VkCommandBuffer cmdBuf, VkQueryPool qp): cmd(cmdBuf), pool(qp)
    {
        vkCmdResetQueryPool(cmd, pool, 0, 2);
        vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,pool,0);
    }

    ~VkGpuTimerScope() {
        vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,pool,1);
    }
};
class VkGpuTimer {
private:
    VkDevice device{};
    VkQueryPool queryPool{};
    uint32_t queryCount = 2;
public:
    VkGpuTimer(VkDevice dev) : device(dev) {
        VkQueryPoolCreateInfo qpci{};
        qpci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        qpci.queryType = VK_QUERY_TYPE_TIMESTAMP;
        qpci.queryCount = queryCount;

        if (vkCreateQueryPool(device, &qpci, nullptr, &queryPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create query pool");
        }
    }

    ~VkGpuTimer() {
        if (queryPool) {
            vkDestroyQueryPool(device, queryPool, nullptr);
        }
    }
    VkQueryPool get_pool() const { return queryPool; }
};

#endif