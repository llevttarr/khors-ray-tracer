#ifndef VK_RENDERER_H
#define VK_RENDERER_H

#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "app_util.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_cmanager.h"

#include "renderer.h"
#include "camera.h"

/**
 * the Vulkan renderer; owns all GPU resources
 */
class VKRenderer : public Renderer{
private:
    std::shared_ptr<VKDevice> device;
    std::shared_ptr<VKSwapchain> swapchain;
    std::shared_ptr<VKCmanager> cmanager;
    EulerCamera& camera;
public:
    VKRenderer(std::shared_ptr<VKDevice> device, std::shared_ptr<VKSwapchain> swapchain,std::shared_ptr<VKCmanager> cmanager, EulerCamera& camera);
    ~VKRenderer();
    VKRenderer(const VKRenderer&) = delete;
    VKRenderer& operator=(const VKRenderer&) = delete;
    void run_rs();
};

#endif // VK_RENDERER_H