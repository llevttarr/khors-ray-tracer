#ifndef VK_G_PIPELINE_H
#define VK_G_PIPELINE_H

#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "app_util.h"
#include "vk_device.h"
#include "vk_swapchain.h"

class VKGraphicsPipeline{
private:
    std::shared_ptr<VKDevice> device;
    std::shared_ptr<VKSwapchain> swapchain;
    
public:

};

#endif // VK_G_PIPELINE_H